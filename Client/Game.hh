#pragma once

#include <Client/Render/Renderer.hh>
#include <Client/Socket.hh>
#include <Client/Ui/Ui.hh>
#include <Client/StaticData.hh>

#include <Shared/Simulation.hh>

#include <array>

namespace Game {
    extern Simulation simulation;
    extern Renderer renderer;
    extern Renderer game_ui_renderer;
    extern Socket socket;
    extern Ui::Window title_ui_window;
    extern Ui::Window game_ui_window;
    extern Ui::Window other_ui_window;
    extern EntityID camera_id;
    extern EntityID player_id;
    extern std::string nickname;
    extern std::string dev_password;
    extern std::string disconnect_message;
    // the page found a login (uuid) to show the server; saves need one
    extern uint8_t has_login;
    // a line shown for a few seconds in the game, and until when (Game::timestamp)
    extern std::string notice;
    extern double notice_until;
    void show_notice(std::string const &);
    // /gardn/#iconsheet: the game draws its icons for the web pages instead of playing
    bool icon_sheet_mode();
    void render_icon_sheet();
    extern std::array<uint8_t, PetalID::kNumPetals> seen_petals;
    extern std::array<uint8_t, MobID::kNumMobs> seen_mobs;
    
    extern double timestamp;
    extern double scale;
    
    extern double score;
    extern float overlevel_timer;
    extern float saving_timer;
    extern float slot_indicator_opacity;
    extern float transition_circle;

    extern uint32_t respawn_level;

    extern std::array<PetalID::T, 2 * MAX_SLOT_COUNT> cached_loadout;

    extern uint8_t loadout_count;
    extern uint8_t simulation_ready;
    extern uint8_t on_game_screen;
    extern uint8_t show_debug;
    extern uint8_t auto_delete;
    // the petals picked in the petal gallery: deleted on touch instead of picked up
    extern std::array<uint8_t, PetalID::kNumPetals> auto_delete_petals;
    // the gallery button: clicking a petal there marks / unmarks it
    extern uint8_t auto_delete_select;
    // auto attack / auto defend (K and L): held as if the button was down while the
    // player is pressing neither. With both on the flower attacks and defends at once
    extern uint8_t hold_attack;
    extern uint8_t hold_defense;
    // the mobs picked in the mob gallery: an observer shows them on the minimap
    extern std::array<uint8_t, MobID::kNumMobs> tracked_mobs;
    // the gallery button: clicking a mob there picks / unpicks it
    extern uint8_t track_select;
    // scroll zoom of the antennae / observer: 0 the normal view, 1 the full view of the petal
    extern float zoom;
    extern uint8_t gamemode;
    // the developer menu of the sandbox (the \ key)
    extern uint8_t show_dev_menu;
    extern uint8_t dev_tab; // 0 petals, 1 mobs
    extern std::string dev_level_text;
    extern std::string dev_score_text;
    extern std::string dev_search_text;

    extern uint8_t show_chat;
    extern std::string chat_text;

    extern uint64_t recovery_id;
    extern uint8_t is_outdated;

    extern uint32_t changelog_entries;
    extern uint32_t seen_changelog;

    extern uint8_t can_restore;
    extern std::vector<struct PlayerProgress> progress_saves;

    void init();
    void reset();
    uint8_t alive();
    uint8_t in_game();
    uint8_t should_render_title_ui();
    uint8_t should_render_game_ui();
    void tick(double);
    void render_game();
    void render_title_screen();
    void send_inputs();
    void spawn_in(uint64_t = 0);
    void delete_petal(uint8_t);
    void swap_petals(uint8_t, uint8_t);
    void swap_all_petals();
    void send_chat(std::string const &);
    void switch_gamemode(uint8_t);
    void save_progress();
    void send_dev_set_level(uint32_t);
    void send_dev_set_score(uint32_t);
    void send_dev_give_petal(uint8_t);
    void send_dev_toggle_invincible();
    void send_dev_spawn_mob(uint8_t);
    void send_dev_clear_mobs();
    void on_message(uint8_t *, uint32_t);
    void poll_ui_event(Ui::ScreenEvent const &);
};
