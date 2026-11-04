#pragma once

#include <Shared/StaticDefinitions.hh>

#include <cstdint>
#include <string>

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
}
