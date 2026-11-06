#pragma once

#include <Shared/StaticDefinitions.hh>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

// Link to the rysteria master server (MasterServer/c in the rrolf repo).
// The WebAssembly server's Node host does the HTTPS calls; the native build has
// no master link, so there every player is anonymous and cannot save progress.
//
// This server keeps no copy of the accounts' saves: the master has them. It is
// asked when something is needed and told when something changes.
//
//  - request_account: asks the master to check "uuid + PL-P" and fetch the
//    account's progress saves (they become the progress entities of the player's
//    camera, nothing else holds them). The answer arrives through Client::on_account.
//  - save: a new progress save, sent at once (a failed send is tried again a few
//    times, that is the only thing the host keeps).
//  - request_restore: asks the master for ONE save and removes it there in the
//    same step. The answer arrives through Client::on_restore.
// A signed-in player in the world, as reported for the crash backup.
struct BackupPlayer {
    std::string uuid;
    uint32_t score;
    std::array<PetalID::T, 2 * MAX_SLOT_COUNT> loadout;
};

namespace Master {
    void init();
    void request_account(int ws_id, std::string const &uuid, std::string const &pl_p);
    void save(std::string const &uuid, struct PlayerProgress const &);
    void request_restore(int ws_id, std::string const &uuid, uint64_t restore_id);
    // for the leaderboards (rrolf/MasterServer/c/leaderboard.c): a kill made by
    // this account. Counted by the host and sent every minute, and on exit. What
    // the saves add up to the master works out itself.
    void add_kill(std::string const &uuid);
    // send everything pending now (shutdown); the host keeps the process alive
    // until the request finishes
    void flush();
    // Crash backup: every 30 seconds the master is told who is in the world, with
    // the score and loadout each one holds. The master keeps exactly this list,
    // so whoever left is dropped, and when this server dies without saying
    // goodbye the list is what the players get back as saves (master: gardn.crash_backups).
    void backup(std::vector<BackupPlayer> const &);
    // the chat command /summon: is `given` the SUMMON_PASSWORD of the server's .env?
    // (false when that is not set, so the command stays off)
    bool summon_password_ok(std::string const &given);
}
