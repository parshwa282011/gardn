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
        GameInstance(Gamemode::kTDM),
        GameInstance(Gamemode::kSandbox)
    };
    std::array<uint32_t, PetalID::kNumPetals> petal_count_tracker = {0};
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

    // One line every 5 minutes: how long the ticks took (the work), and how far apart
    // they started (the work plus what the machine did in between). A tick is "over" when it
    // took more than the 50 ms it has; a "gap" is a start more than 75 ms after the last
    static auto last_start = start;
    static uint32_t stat_ticks = 0, stat_over = 0, stat_gaps = 0;
    static double stat_sum = 0, stat_max = 0;
    static auto stat_since = start;
    std::chrono::duration<double, std::milli> gap = start - last_start;
    last_start = start;
    ++stat_ticks;
    stat_sum += tick_time.count();
    if (tick_time.count() > stat_max) stat_max = tick_time.count();
    if (tick_time > 1000ms / TPS) ++stat_over;
    if (stat_ticks > 1 && gap > 1500ms / TPS) ++stat_gaps;
    if (stat_ticks >= 300 * TPS) {
        std::chrono::duration<double> span = end - stat_since;
        std::cout << "tick stats: " << stat_ticks << " ticks, avg " << stat_sum / stat_ticks
            << " ms, max " << stat_max << " ms, over 50 ms: " << stat_over
            << ", late starts: " << stat_gaps << ", " << stat_ticks / span.count()
            << " ticks/s, players " << Server::player_count << '\n';
        stat_ticks = stat_over = stat_gaps = 0;
        stat_sum = stat_max = 0;
        stat_since = end;
    }

    // the crash backup: who is in the world, every 30 seconds. Nothing is sent while
    // nobody is in (a standby has no players: it must not wipe the primary's backups
    // that were copied to it), except the one list that says the last player left
    static uint32_t backup_ticks = 0;
    static bool backup_had_players = false;
    if (!Server::is_stopping && ++backup_ticks >= 30 * TPS) {
        backup_ticks = 0;
        std::vector<BackupPlayer> list;
        for (GameInstance &game : Server::games) game.collect_backup(list);
        if (!list.empty() || backup_had_players) {
            Master::backup(list);
            backup_had_players = !list.empty();
        }
    }

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
