#include <Server/Client.hh>

#include <Server/EntityFunctions.hh>
#include <Server/Game.hh>
#include <Server/Master.hh>
#include <Server/PetalTracker.hh>
#include <Server/Server.hh>
#include <Server/Spawn.hh>

#include <Helpers/UTF8.hh>

#include <Shared/Binary.hh>
#include <Shared/Config.hh>

#include <array>
#include <iostream>

Client::Client() : game(nullptr) {}

void Client::init(uint64_t recovery_id, uint8_t gamemode, std::vector<PlayerProgress> const &saves) {
    DEBUG_ONLY(assert(game == nullptr);)
    DEBUG_ONLY(recovery_id = 0;)
    EntityID camera_id = NULL_ENTITY;
    for (GameInstance &game : Server::games) {
        game.simulation.for_each<kCamera>([&](Simulation *sim, Entity &ent){
            if (ent.get_recovery_id() == recovery_id) {
                assert(camera_id == NULL_ENTITY);
                camera_id = ent.id;
                gamemode = game.gamemode;
            }
        });
    }
    if (camera_id == NULL_ENTITY && Server::is_draining) {
        disconnect(CloseReason::kOutdated, "Outdated Version");
        return;
    }
    Server::games[gamemode].add_client(this, camera_id);
    if (camera_id == NULL_ENTITY)
        game->show_progress_saves(this, saves);
}

void Client::move(GameInstance *new_game) {
    if (game == new_game) return;
    Entity &old_camera = game->simulation.get_ent(camera);
    // the saves are the entities of the old camera: they move with the player
    std::vector<PlayerProgress> saves = game->progress_of(this);
    game->simulation.request_delete(camera);
    new_game->add_client(this, NULL_ENTITY);
    game->show_progress_saves(this, saves);
    Entity &new_camera = game->simulation.get_ent(camera);
    new_camera.set_respawn_level(old_camera.get_respawn_level());
    for (uint32_t i = 0; i < 2 * MAX_SLOT_COUNT; ++i) {
        PetalTracker::remove_petal(new_camera.get_inventory(i));
        new_camera.set_inventory(i, old_camera.get_inventory(i));
        PetalTracker::add_petal(new_camera.get_inventory(i));
    }
    in_view.clear();
    seen_arena = 0;
}

void Client::remove() {
    if (game == nullptr) return;
    game->remove_client(this);
}

void Client::disconnect(int reason, std::string const &message) {
    if (ws == nullptr) return;
    remove();
    ws->end(reason, message);
}

uint8_t Client::alive() {
    if (game == nullptr) return false;
    Simulation *simulation = &game->simulation;
    return simulation->ent_exists(camera) 
    && simulation->ent_exists(simulation->get_ent(camera).get_player());
}

void Client::on_message(WebSocket *ws, std::string_view message, uint64_t code) {
    if (ws == nullptr) return;
    uint8_t const *data = reinterpret_cast<uint8_t const *>(message.data());
    Reader reader(data);
    Validator validator(data, data + message.size());
    Client *client = ws->getUserData();
    if (client == nullptr) {
        ws->end(CloseReason::kServer, "Server Error");
        return;
    }
    if (client->verifying) return; // the master is checking the account
    if (!client->verified) {
        if (client->check_invalid(
            validator.validate_uint8() &&
            validator.validate_uint64()
        )) return;
        if (reader.read<uint8_t>() != Serverbound::kVerify) {
            client->disconnect();
            return;
        }
        if (reader.read<uint64_t>() != VERSION_HASH) {
            client->disconnect(CloseReason::kOutdated, "Outdated Version");
            return;
        }
        if (client->check_invalid(
            validator.validate_uint64() &&
            validator.validate_uint8()
        )) return;
        uint64_t recovery_id = reader.read<uint64_t>();
        uint8_t gamemode = reader.read<uint8_t>();
        if (client->check_invalid(gamemode < Gamemode::kNumGamemodes)) return;
        // who is playing: the uuid and PL-P from the rrolf login (localStorage
        // on the shared origin). Both empty = anonymous.
        if (client->check_invalid(validator.validate_string(MAX_UUID_LENGTH))) return;
        std::string uuid;
        reader.read<std::string>(uuid);
        if (client->check_invalid(validator.validate_string(MAX_PLP_LENGTH))) return;
        std::string pl_p;
        reader.read<std::string>(pl_p);
#ifdef WASM_SERVER
        if (!uuid.empty()) {
            client->verifying = 1;
            client->account_uuid = uuid;
            client->pending_recovery_id = recovery_id;
            client->pending_gamemode = gamemode;
            Master::request_account(ws->id(), uuid, pl_p);
            return;
        }
#endif
        client->verified = 1;
        client->init(recovery_id, gamemode);
        return;
    }
    if (client->game == nullptr) {
        client->disconnect(CloseReason::kServer, "Server Error");
        return;
    }
    if (client->check_invalid(validator.validate_uint8())) return;
    switch (reader.read<uint8_t>()) {
        case Serverbound::kVerify:
            client->disconnect();
            return;
        case Serverbound::kClientInput: {
            if (!client->alive()) break;
            Simulation *simulation = &client->game->simulation;
            Entity &camera = simulation->get_ent(client->camera);
            Entity &player = simulation->get_ent(camera.get_player());
            if (client->check_invalid(
                validator.validate_float() &&
                validator.validate_float() &&
                validator.validate_uint8() &&
                validator.validate_uint8()
            )) return;
            float x = reader.read<float>();
            float y = reader.read<float>();
            if (x == 0 && y == 0) player.acceleration.set(0,0);
            else {
                if (std::abs(x) > 5e3 || std::abs(y) > 5e3) break;
                Vector accel(x,y);
                float m = accel.magnitude();
                if (m > 200) accel.set_magnitude(PLAYER_ACCELERATION);
                else accel.set_magnitude(m / 200 * PLAYER_ACCELERATION);
                player.acceleration = accel;
            }
            player.input = reader.read<uint8_t>();
            player.settings = reader.read<uint8_t>();
            break;
        }
        case Serverbound::kClientSpawn: {
            if (client->alive() || client->restoring) break;
            //check string length
            std::string name, pwd;
            if (client->check_invalid(validator.validate_uint64())) return;
            if (client->check_invalid(validator.validate_string(MAX_NAME_LENGTH))) return;
            if (client->check_invalid(validator.validate_string(MAX_DEV_PWD_LENGTH))) return;
            uint64_t restore_id = reader.read<uint64_t>();
            reader.read<std::string>(name);
            reader.read<std::string>(pwd);
            if (client->check_invalid(UTF8Parser::is_valid_utf8(name))) return;
            for (char &c : pwd) c ^= pwd.capacity();
            uint8_t dev = pwd == "ez hax"; // feel free to use
#ifdef WASM_SERVER
            if (restore_id != 0 && !client->account_uuid.empty()) {
                // the save is handed over by the master (and gone from there)
                if (client->restoring) break;
                client->restoring = 1;
                client->restoring_name = name;
                client->restoring_dev = dev;
                client->restoring_id = restore_id;
                Master::request_restore(ws->id(), client->account_uuid, restore_id);
                break;
            }
#endif
            client->spawn(name, dev, nullptr);
            break;
        }
        case Serverbound::kPetalDelete: {
            if (!client->alive()) break;
            Simulation *simulation = &client->game->simulation;
            Entity &camera = simulation->get_ent(client->camera);
            Entity &player = simulation->get_ent(camera.get_player());
            if (client->check_invalid(validator.validate_uint8())) return;
            uint8_t pos = reader.read<uint8_t>();
            if (pos >= MAX_SLOT_COUNT + player.get_loadout_count()) break;
            PetalID::T old_id = player.get_loadout_ids(pos);
            delete_petal(simulation, player, old_id);
            player.set_loadout_ids(pos, PetalID::kNone);
            break;
        }
        case Serverbound::kPetalSwap: {
            if (!client->alive()) break;
            Simulation *simulation = &client->game->simulation;
            Entity &camera = simulation->get_ent(client->camera);
            Entity &player = simulation->get_ent(camera.get_player());
            if (client->check_invalid(validator.validate_uint8() && validator.validate_uint8())) return;
            uint8_t pos1 = reader.read<uint8_t>();
            if (pos1 >= MAX_SLOT_COUNT + player.get_loadout_count()) break;
            uint8_t pos2 = reader.read<uint8_t>();
            if (pos2 >= MAX_SLOT_COUNT + player.get_loadout_count()) break;
            PetalID::T tmp = player.get_loadout_ids(pos1);
            player.set_loadout_ids(pos1, player.get_loadout_ids(pos2));
            player.set_loadout_ids(pos2, tmp);
            if (pos1 < player.get_loadout_count()) player.set_loadout_reloads(pos1, 0);
            if (pos2 < player.get_loadout_count()) player.set_loadout_reloads(pos2, 0);
            break;
        }
        case Serverbound::kChatSend: {
            if (!client->alive()) break;
            std::string text;
            if (client->check_invalid(validator.validate_string(MAX_CHAT_LENGTH))) return;
            reader.read<std::string>(text);
            if (client->check_invalid(UTF8Parser::is_valid_utf8(text))) return;
            // text = UTF8Parser::trunc_string(text, MAX_CHAT_LENGTH);
            if (text.size() == 0) break;
            Simulation *simulation = &client->game->simulation;
            Entity &camera = simulation->get_ent(client->camera);
            Entity &player = simulation->get_ent(camera.get_player());
            if (player.chat_sent != NULL_ENTITY) break;
            player.chat_sent = alloc_chat(simulation, text, player).id;
            std::cout << "chat " << name_or_unnamed(player.get_name()) << ": " << text << '\n';
            break;
        }
        case Serverbound::kGamemodeSwitch: {
            if (client->alive()) break;
            if (client->check_invalid(validator.validate_uint8())) return;
            uint8_t gamemode = reader.read<uint8_t>();
            if (client->check_invalid(gamemode < Gamemode::kNumGamemodes)) return;
            client->move(&Server::games[gamemode]);
            break;
        }
        case Serverbound::kProgressSave: {
            if (!client->alive()) break;
            if (client->account_uuid.empty()) break;
            Simulation *simulation = &client->game->simulation;
            Entity &camera = simulation->get_ent(client->camera);
            if (!simulation->ent_alive(camera.get_player())) break;
            camera.set_saving_progress(!camera.get_saving_progress());
            break;
        }
    }
}

void Client::on_disconnect(WebSocket *ws, int code, std::string_view message) {
    std::printf("disconnect: [%d]\n", code);
    Client *client = ws->getUserData();
    if (client == nullptr) return;
    client->remove();
}

void Client::on_account(uint8_t status, std::vector<PlayerProgress> &saves) {
    verifying = 0;
    if (status == 2) { disconnect(CloseReason::kExpired, "Session Expired"); return; }
    if (status == 3) { disconnect(CloseReason::kServer, "Server Error"); return; }
    if (status != 1) { disconnect(CloseReason::kUnauthorized, "Not Logged In"); return; }
    verified = 1;
    // the list is only passed on: the camera gets one entity per save and this
    // server holds on to nothing else
    init(pending_recovery_id, pending_gamemode, saves);
}

void Client::on_restore(uint8_t status, PlayerProgress const *progress) {
    if (!restoring) return;
    restoring = 0;
    if (game == nullptr || alive()) {
        // nowhere to put it: give the save back to the master
        if (status == 1 && progress != nullptr) Master::save(account_uuid, *progress);
        return;
    }
    // gone from the master: the entity on the player's screen is stale
    if (status == 4) game->drop_progress(this, restoring_id);
    // could not ask the master: the save is still there, spawn without it
    spawn(restoring_name, restoring_dev, status == 1 ? progress : nullptr);
    restoring_name.clear();
}

void Client::spawn(std::string const &name, uint8_t dev, PlayerProgress const *progress) {
    Simulation *simulation = &game->simulation;
    Entity &camera = simulation->get_ent(this->camera);
    uint32_t score = progress != nullptr ? game->apply_progress(this, *progress) : 0;
    Entity &player = alloc_player(simulation, camera.get_team());
    player_spawn(simulation, camera, player);
    if (score != 0) player.set_score(score);
    player.set_name(name);
    camera.set_dev(dev);
    player.set_dev(dev);
    std::cout << "player_spawn" << (dev ? "_dev " : " ") << name_or_unnamed(name)
        << " <" << +game->gamemode << "," << +player.id.hash << "," << +player.id.id << ">\n";
}

bool Client::check_invalid(bool valid) {
    if (valid) return false;
    std::cout << "client sent an invalid packet\n";
    //optional
    disconnect();

    return true;
}