#include <Server/Server.hh>

#include <Server/Game.hh>
#include <Server/Client.hh>
#include <Server/Master.hh>
#include <Server/PetalTracker.hh>

#include <Shared/Binary.hh>

#include <chrono>
#include <iostream>

static bool was_draining = false;

namespace Server {
    uint8_t OUTGOING_PACKET[MAX_PACKET_LEN] = {0};
    std::array<GameInstance, Gamemode::kNumGamemodes> games = {
        GameInstance(Gamemode::kFFA),
        GameInstance(Gamemode::kTDM)
    };
    std::array<uint32_t, PetalID::kNumPetals> petal_count_tracker = {0};
    std::unordered_map<std::string, AccountSaves> account_saves = {};

    void report_stats(std::string const &uuid) {
        auto account = account_saves.find(uuid);
        if (account == account_saves.end()) return;
        uint32_t mythics = 0, best = 0;
        for (PlayerProgress const &progress : account->second.saves) {
            if (progress.score > best) best = progress.score;
            for (PetalID::T id : progress.loadout)
                if (id != PetalID::kNone && PETAL_DATA[id].rarity == RarityID::kMythic) ++mythics;
        }
        Master::set_stats(uuid, mythics, best);
    }
    volatile sig_atomic_t is_draining = false;
    bool is_stopping = false;
    uint32_t player_count = 0;
}

using namespace Server;

void Server::tick() {
    using namespace std::chrono_literals;
    auto start = std::chrono::steady_clock::now();
    for (GameInstance &game : Server::games) game.tick();
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> tick_time = end - start;
    if (tick_time > 1000ms / TPS) std::cout << "tick took " << tick_time << '\n';

    if (Server::is_draining && !was_draining) {
        was_draining = true;
        std::cout << "draining...\n";
    }
    if (Server::is_draining && !Server::is_stopping && Server::player_count == 0) {
        Server::stop();
        #ifndef DEBUG
        for (PetalID::T id = 0; id < PetalID::kNumPetals; ++id) {
            assert(PetalTracker::get_count(id) == 0);
        }
        #endif
    }
}
