#include <Server/Game.hh>

#include <Server/Client.hh>
#include <Server/EntityFunctions.hh>
#include <Server/Master.hh>
#include <Server/PetalTracker.hh>
#include <Server/Server.hh>
#include <Server/Spawn.hh>

#include <Shared/Binary.hh>
#include <Shared/Entity.hh>
#include <Shared/Map.hh>

static void _update_client(Simulation *sim, Client *client) {
    if (client == nullptr) return;
    if (!client->verified) return;
    if (sim == nullptr) return;
    if (!sim->ent_exists(client->camera)) return;
    std::set<EntityID> in_view;
    std::vector<EntityID> deletes;
    in_view.insert(client->camera);
    Entity &camera = sim->get_ent(client->camera);
    if (sim->ent_exists(camera.get_player())) 
        in_view.insert(camera.get_player());
    if (sim->arena_info.gamemode == Gamemode::kTDM) {
        Entity &team = sim->get_ent(camera.get_team());
        in_view.insert(team.minimap_dots.begin(), team.minimap_dots.end());
    }
    for (EntityID dot_id : sim->arena_info.leader_dots) {
        if (sim->get_ent(dot_id).get_team() != camera.get_team())
            in_view.insert(dot_id);
    }
    in_view.insert(camera.progress_saves.begin(), camera.progress_saves.end());
    Writer writer(Server::OUTGOING_PACKET);
    writer.write<uint8_t>(Clientbound::kClientUpdate);
    writer.write<uint8_t>(client->seen_arena);
    writer.write<uint8_t>(Server::is_draining);
    writer.write<EntityID>(client->camera);
    sim->spatial_hash.query(camera.get_camera_x(), camera.get_camera_y(), 
    960 / camera.get_fov() + 100, 540 / camera.get_fov() + 100, 
    [&](Simulation *, Entity &ent){
        in_view.insert(ent.id);
        if (ent.has_component(kSegmented) && ent.has_component(kAnimation)) {
            if (sim->ent_exists(ent.get_seg_head())) in_view.insert(ent.get_seg_head());
            if (sim->ent_exists(ent.get_seg_tail())) in_view.insert(ent.get_seg_tail());
        }
    });

    for (EntityID const &i: client->in_view) {
        if (!in_view.contains(i)) {
            writer.write<EntityID>(i);
            deletes.push_back(i);
        }
    }

    for (EntityID const &i : deletes)
        client->in_view.erase(i);

    writer.write<EntityID>(NULL_ENTITY);
    //upcreates
    for (EntityID id: in_view) {
        DEBUG_ONLY(assert(sim->ent_exists(id));)
        Entity &ent = sim->get_ent(id);
        uint8_t create = !client->in_view.contains(id);
        writer.write<EntityID>(id);
        writer.write<uint8_t>(create | (ent.pending_delete << 1));
        ent.write(&writer, BitMath::at(create, 0));
        client->in_view.insert(id);
    }
    writer.write<EntityID>(NULL_ENTITY);
    //write arena stuff
    sim->arena_info.write(&writer, !client->seen_arena);
    client->seen_arena = 1;
    client->send_packet(writer.packet, writer.at - writer.packet);
}

GameInstance::GameInstance(uint8_t mode) : simulation(), clients(), team_manager(&simulation), gamemode(mode) {}

void GameInstance::init() {
    simulation.arena_info.set_gamemode(gamemode);
    for (uint32_t i = 0; i < ENTITY_CAP / 2; ++i)
        Map::spawn_random_mob(&simulation, frand() * ARENA_WIDTH, frand() * ARENA_HEIGHT);
    if (gamemode == Gamemode::kTDM) {
        team_manager.add_team(ColorID::kBlue);
        team_manager.add_team(ColorID::kRed);
    }
}

void GameInstance::tick() {
    simulation.tick();
    if (gamemode == Gamemode::kTDM)
        team_manager.tick();
    for (Client *client : clients)
        _update_client(&simulation, client);
    simulation.post_tick();
}

void GameInstance::add_client(Client *client, EntityID camera_id) {
    DEBUG_ONLY(assert(client->game != this);)
    if (client->game != nullptr)
        client->game->remove_client(client);
    client->game = this;
    clients.insert(client);
    if (camera_id == NULL_ENTITY) {
        EntityID team;
        if (gamemode == Gamemode::kTDM)
            team = team_manager.get_random_team();
        client->camera = alloc_camera(&simulation, team).id;
    } else
        client->camera = camera_id;
    Entity &camera = simulation.get_ent(client->camera);
    if (camera.client != nullptr)
        camera.client->disconnect(CloseReason::kRecovered, "Session Recovered");
    camera.client = client;
    BitMath::unset(camera.flags, EntityFlags::kZombie);
    BitMath::unset(camera.flags, EntityFlags::kIsDespawning);
}

void GameInstance::remove_client(Client *client) {
    DEBUG_ONLY(assert(client->game == this);)
    clients.erase(client);
    if (simulation.ent_exists(client->camera)) {
        Entity &camera = simulation.get_ent(client->camera);
        DEBUG_ONLY(simulation.request_delete(camera.id);)
        BitMath::set(camera.flags, EntityFlags::kZombie);
        camera.client = nullptr;
    }
    client->game = nullptr;
}

void GameInstance::save_progress(Client *client) {
    // saves belong to accounts; anonymous players cannot make them
    if (client->account_uuid.empty()) return;
    Entity &camera = simulation.get_ent(client->camera);
    Entity &player = simulation.get_ent(camera.get_player());
    player.last_damaged_by = NULL_ENTITY;
    BitMath::set(player.flags, EntityFlags::kIsDespawning);
    simulation.request_delete(player.id);

    uint64_t restore_id = 0;
    while (restore_id == 0)
        restore_id = (static_cast<uint64_t>(std::rand()) << 32) | std::rand();
    std::array<PetalID::T, 2 * MAX_SLOT_COUNT> loadout = {};
    for (uint8_t i = 0; i < player.get_loadout_count() + MAX_SLOT_COUNT; ++i)
        loadout[i] = player.get_loadout_ids(i);
    struct PlayerProgress progress = { .restore_id = restore_id, .score = player.get_score(), .loadout = loadout };
    Server::account_saves[client->account_uuid].saves.push_back(progress);
    Master::queue_add(client->account_uuid, progress);
    DEBUG_ONLY(assert(camera.progress_saves.size() < MAX_PROGRESS_SAVES);)
    camera.progress_saves.push_back(alloc_progress(&simulation, progress, camera).id);
}

uint32_t GameInstance::restore_progress(Client *client, uint64_t restore_id) {
    if (client->account_uuid.empty()) return 0;
    Entity &camera = simulation.get_ent(client->camera);
    auto account = Server::account_saves.find(client->account_uuid);
    if (account == Server::account_saves.end()) return 0;
    std::vector<PlayerProgress> &saves = account->second.saves;
    auto iter = std::find_if(saves.begin(), saves.end(), [restore_id](PlayerProgress const &p){ return p.restore_id == restore_id; });
    if (iter == saves.end())
        return 0;

    struct PlayerProgress progress = *iter;
    camera.set_respawn_level(score_to_level(progress.score));
    for (uint32_t i = 0; i < 2 * MAX_SLOT_COUNT; ++i)
        PetalTracker::remove_petal(camera.get_inventory(i));
    for (uint32_t i = 0; i < 2 * MAX_SLOT_COUNT; ++i) {
        PetalID::T id = progress.loadout[i];
        if (PETAL_DATA[id].rarity == RarityID::kUnique && PetalTracker::get_count(id) > 0)
            id = PetalID::kNone;
        camera.set_inventory(i, id);
        PetalTracker::add_petal(id);
    }

    saves.erase(iter);
    Master::queue_remove(client->account_uuid, restore_id);
    simulation.for_each<kProgress>([=](Simulation *sim, Entity &ent){
        if (ent.get_restore_id() == restore_id) {
            sim->request_delete(ent.id);
            std::erase(sim->get_ent(ent.get_parent()).progress_saves, ent.id);
        }
    });

    return progress.score;
}

void GameInstance::find_progress_saves(Client *client) {
    Entity &camera = simulation.get_ent(client->camera);
    DEBUG_ONLY(assert(camera.progress_saves.size() == 0);)
    if (client->account_uuid.empty()) return;
    auto account = Server::account_saves.find(client->account_uuid);
    if (account == Server::account_saves.end()) return;
    for (struct PlayerProgress &progress : account->second.saves) {
        if (camera.progress_saves.size() >= MAX_PROGRESS_SAVES) break;
        camera.progress_saves.push_back(alloc_progress(&simulation, progress, camera).id);
    }
}
