#pragma once

#include <cstdint>
#include <string>

namespace DOM {
    void create_text_input(char const *, uint32_t);
    void element_show(char const *);
    void element_hide(char const *);
    void update_pos_and_dimension(char const *, float, float, float, float);
    std::string retrieve_text(char const *, uint32_t);
    void update_text(char const *, std::string const &, uint32_t);
    void element_focus(char const *);
    void blur();
    void open_page(char const *);
    void update_client(float);
    void cache_client();
    void toggle_fullscreen();
    uint8_t text_input_focused();
    // The best scores of gardn (the master's /leaderboard, the same numbers as the leaderboard
    // page). fetch_high_scores asks in the background; retrieve_high_scores gives what the
    // last answer had, one line per player ("1. name - score"), empty before there is one
    // Proximity voice chat (the script is in index.html, window.gardnVoice): voice_init links it
    // to the game, voice_toggle turns the mic on / off, voice_update gives it who is around
    // ("code:distance:talking,...") and voice_signal_in a piece of a handshake
    void voice_init();
    void voice_toggle();
    void voice_update(uint32_t, std::string const &);
    void voice_signal_in(uint32_t, std::string const &);
    void fetch_high_scores();
    std::string retrieve_high_scores();
}