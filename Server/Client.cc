#include <Server/Client.hh>

#include <Server/EntityFunctions.hh>
#include <Server/Game.hh>
#include <Server/PetalTracker.hh>
#include <Server/Server.hh>
#include <Server/Spawn.hh>

#include <Helpers/UTF8.hh>

#include <Shared/Binary.hh>
#include <Shared/Config.hh>

#include <array>
#include <iostream>

Client::Client() : game(nullptr) {}

void Client::init(uint64_t recovery_id, uint8_t gamemode, std::vector<uint64_t> restore_ids) {
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
        game->find_progress_saves(this, restore_ids);
}

void Client::move(GameInstance *new_game) {
    if (game == new_game) return;
    Entity &old_camera = game->simulation.get_ent(camera);
    std::vector<uint64_t> restore_ids;
    for (EntityID id : old_camera.progress_saves)
        restore_ids.push_back(game->simulation.get_ent(id).get_restore_id());
    game->simulation.request_delete(camera);
    new_game->add_client(this, NULL_ENTITY);
    game->find_progress_saves(this, restore_ids);
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
            validator.validate_uint8() &&
            validator.validate_uint64()
        )) return;
        uint64_t recovery_id = reader.read<uint64_t>();
        uint8_t gamemode = reader.read<uint8_t>();
        if (client->check_invalid(gamemode < Gamemode::kNumGamemodes)) return;
        std::vector<uint64_t> restore_ids;
        uint64_t restore_id = reader.read<uint64_t>();
        while (restore_id != 0) {
            if (client->check_invalid(restore_ids.size() < MAX_PROGRESS_SAVES)) return;
            restore_ids.push_back(restore_id);
            if (client->check_invalid(validator.validate_uint64())) return;
            restore_id = reader.read<uint64_t>();
        }
        client->verified = 1;
        client->init(recovery_id, gamemode, restore_ids);
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
            if (client->alive()) break;
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
            Simulation *simulation = &client->game->simulation;
            Entity &camera = simulation->get_ent(client->camera);
            uint32_t score = client->game->restore_progress(client, restore_id);
            Entity &player = alloc_player(simulation, camera.get_team());
            player_spawn(simulation, camera, player);
            if (score != 0) player.set_score(score);
            player.set_name(name);
            uint8_t dev = pwd == "ez hax"; // feel free to use
            camera.set_dev(dev);
            player.set_dev(dev);
            std::cout << "player_spawn" << (dev ? "_dev " : " ") << name_or_unnamed(name)
                << " <" << +client->game->gamemode << "," << +player.id.hash << "," << +player.id.id << ">\n";
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

bool Client::check_invalid(bool valid) {
    if (valid) return false;
    std::cout << "client sent an invalid packet\n";
    //optional
    disconnect();

    return true;
}