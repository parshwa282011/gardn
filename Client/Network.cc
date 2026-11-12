#include <Client/DOM.hh>
#include <format>
#include <Client/Game.hh>

#include <Client/Input.hh>
#include <Client/Ui/Ui.hh>

#include <Shared/Binary.hh>
#include <Shared/Config.hh>

using namespace Game;

void Game::on_message(uint8_t *ptr, uint32_t len) {
    Reader reader(ptr);
    switch(reader.read<uint8_t>()) {
        case Clientbound::kClientUpdate: {
            uint8_t seen_arena = reader.read<uint8_t>();
            if (!seen_arena) Game::reset();
            simulation_ready = 1;
            is_outdated = reader.read<uint8_t>();
            camera_id = reader.read<EntityID>();
            EntityID curr_id = reader.read<EntityID>();
            while(!(curr_id == NULL_ENTITY)) {
                assert(simulation.ent_exists(curr_id));
                Entity &ent = simulation.get_ent(curr_id);
                simulation._delete_ent(curr_id);
                curr_id = reader.read<EntityID>();
            }
            curr_id = reader.read<EntityID>();
            while(!(curr_id == NULL_ENTITY)) {
                uint8_t create = reader.read<uint8_t>();
                if (BitMath::at(create, 0)) simulation.force_alloc_ent(curr_id);
                assert(simulation.ent_exists(curr_id));
                Entity &ent = simulation.get_ent(curr_id);
                ent.read(&reader, BitMath::at(create, 0));
                if (BitMath::at(create, 1)) ent.pending_delete = 1;
                curr_id = reader.read<EntityID>();
            }
            simulation.arena_info.read(&reader, !seen_arena);
            break;
        }
        default:
            break;
    }
}

void Game::send_inputs() {
    Writer writer(static_cast<uint8_t *>(OUTGOING_PACKET));
    writer.write<uint8_t>(Serverbound::kClientInput);
    if (Input::freeze_input) {
        // the flower stands still, but the petals still follow the buttons (mouse or
        // SPACE/SHIFT), so they can be spread and pulled in from the box as well
        writer.write<float>(0);
        writer.write<float>(0);
        writer.write<uint8_t>(Input::game_inputs.flags);
    } else {
        writer.write<float>(Input::game_inputs.x);
        writer.write<float>(Input::game_inputs.y);
        writer.write<uint8_t>(Input::game_inputs.flags);
    }
    uint8_t settings = 0;
    if (Game::auto_delete) BitMath::set(settings, SettingFlags::kAutoDelete);
    writer.write<uint8_t>(settings);
    writer.write<uint8_t>((uint8_t) (fclamp(Game::zoom, 0, 1) * 255 + 0.5f));
    for (uint32_t i = 0; i < AUTO_DELETE_MASK_BYTES; ++i) {
        uint8_t byte = 0;
        for (uint32_t bit = 0; bit < 8 && i * 8 + bit < PetalID::kNumPetals; ++bit)
            if (Game::auto_delete_petals[i * 8 + bit]) BitMath::set(byte, bit);
        writer.write<uint8_t>(byte);
    }
    for (uint32_t i = 0; i < TRACKED_MOB_MASK_BYTES; ++i) {
        uint8_t byte = 0;
        for (uint32_t bit = 0; bit < 8 && i * 8 + bit < MobID::kNumMobs; ++bit)
            if (Game::tracked_mobs[i * 8 + bit]) BitMath::set(byte, bit);
        writer.write<uint8_t>(byte);
    }
    socket.send(writer.packet, writer.at - writer.packet);
}

void Game::spawn_in(uint64_t restore_id) {
    Writer writer(static_cast<uint8_t *>(OUTGOING_PACKET));
    if (Game::alive()) return;
    if (Game::on_game_screen == 0) {
        if (Game::gamemode != Gamemode::kSandbox)
            Game::show_notice(std::format("Spawn protection for {} seconds: nothing collides with you or targets you", SPAWN_PROTECTION_SECONDS));
        writer.write<uint8_t>(Serverbound::kClientSpawn);
        writer.write<uint64_t>(restore_id);
        writer.write<std::string>(Game::nickname);
        writer.write<std::string>(Game::dev_password);
        socket.send(writer.packet, writer.at - writer.packet);
    } else Game::on_game_screen = 0;
}

void Game::delete_petal(uint8_t pos) {
    Writer writer(static_cast<uint8_t *>(OUTGOING_PACKET));
    if (!Game::alive()) return;
    writer.write<uint8_t>(Serverbound::kPetalDelete);
    writer.write<uint8_t>(pos);
    socket.send(writer.packet, writer.at - writer.packet);
}

void Game::swap_petals(uint8_t pos1, uint8_t pos2) {
    Writer writer(static_cast<uint8_t *>(OUTGOING_PACKET));
    if (!Game::alive()) return;
    writer.write<uint8_t>(Serverbound::kPetalSwap);
    writer.write<uint8_t>(pos1);
    writer.write<uint8_t>(pos2);
    socket.send(writer.packet, writer.at - writer.packet);
}

void Game::swap_all_petals() {
    if (!Game::alive()) return;
    for (uint32_t i = 0; i < Game::loadout_count; ++i)
        Ui::ui_swap_petals(i, i + Game::loadout_count);
}

void Game::send_chat(std::string const &text) {
    Writer writer(static_cast<uint8_t *>(OUTGOING_PACKET));
    if (!Game::alive()) return;
    writer.write<uint8_t>(Serverbound::kChatSend);
    writer.write<std::string>(text);
    socket.send(writer.packet, writer.at - writer.packet);
}

void Game::switch_gamemode(uint8_t gamemode) {
    Writer writer(static_cast<uint8_t *>(OUTGOING_PACKET));
    // only the way out of the sandbox works while playing (the X)
    if (Game::alive() && !(Game::simulation.arena_info.gamemode == Gamemode::kSandbox && gamemode != Gamemode::kSandbox)) return;
    writer.write<uint8_t>(Serverbound::kGamemodeSwitch);
    writer.write<uint8_t>(gamemode);
    socket.send(writer.packet, writer.at - writer.packet);
}

void Game::save_progress() {
    Writer writer(static_cast<uint8_t *>(OUTGOING_PACKET));
    if (!Game::alive()) return;
    if (!Game::has_login) {
        Game::show_notice("Not logged in: saves are off. Log in on the Parteria page, then open Gardn from its button.");
        return;
    }
    writer.write<uint8_t>(Serverbound::kProgressSave);
    socket.send(writer.packet, writer.at - writer.packet);
}

// the developer menu: the server only listens to these in the sandbox
static void send_dev(uint8_t command, uint32_t arg, bool has_arg, bool big = false) {
    Writer writer(static_cast<uint8_t *>(OUTGOING_PACKET));
    if (!Game::alive()) return;
    writer.write<uint8_t>(Serverbound::kDevCommand);
    writer.write<uint8_t>(command);
    if (has_arg) {
        if (big) writer.write<uint32_t>(arg);
        else writer.write<uint8_t>(arg);
    }
    Game::socket.send(writer.packet, writer.at - writer.packet);
}

void Game::send_dev_set_level(uint32_t level) { send_dev(DevCommandID::kSetLevel, level, true); }
void Game::send_dev_set_score(uint32_t score) { send_dev(DevCommandID::kSetScore, score, true, true); }
void Game::send_dev_give_petal(uint8_t id) { send_dev(DevCommandID::kGivePetal, id, true); }
void Game::send_dev_toggle_invincible() { send_dev(DevCommandID::kToggleInvincible, 0, false); }
void Game::send_dev_spawn_mob(uint8_t id) { send_dev(DevCommandID::kSpawnMob, id, true); }
void Game::send_dev_clear_mobs() { send_dev(DevCommandID::kClearMobs, 0, false); }
