#pragma once

#include <Server/Game.hh>

#include <set>
#include <csignal>

class Client;

size_t const MAX_PACKET_LEN = 64 * 1024;

#ifdef WASM_SERVER
class WebSocketServer {
public:
    WebSocketServer();
};
#else
#include <App.h>
typedef uWS::App WebSocketServer;
#endif

namespace Server {
    extern uint8_t OUTGOING_PACKET[MAX_PACKET_LEN];
    extern std::array<GameInstance, Gamemode::kNumGamemodes> games;
    extern std::array<uint32_t, PetalID::kNumPetals> petal_count_tracker;
    // Progress saves belong to accounts. An entry exists while at least one of
    // the account's clients is connected; unflushed changes live in the host
    // (see Master.hh), which flushes before the next load.
    struct AccountSaves {
        std::vector<struct PlayerProgress> saves;
        uint32_t clients = 0;
    };
    extern std::unordered_map<std::string, AccountSaves> account_saves;
    #ifdef WASM_SERVER
    extern WebSocketServer server;
    #endif
    extern volatile sig_atomic_t is_draining;
    extern bool is_stopping;
    extern uint32_t player_count;
    extern void init();
    extern void run();
    extern void tick();
    extern void stop();
};