#pragma once

#include <Shared/StaticDefinitions.hh>

#include <cstdint>
#include <string>

// Link to the rysteria master server (MasterServer/c in the rrolf repo).
// The WebAssembly server's Node host does the HTTPS calls; the native build has
// no master link, so there every player is anonymous and cannot save progress.
//
//  - request_account: asks the master to check "uuid + PL-P" and fetch the
//    account's progress saves. The answer arrives through Client::on_account.
//  - queue_add / queue_remove: progress save changes. The host sends them in
//    one batch every 60 seconds, immediately before loading the same account
//    again, and on exit / drain.
namespace Master {
    void init();
    void request_account(int ws_id, std::string const &uuid, std::string const &pl_p);
    void queue_add(std::string const &uuid, struct PlayerProgress const &);
    void queue_remove(std::string const &uuid, uint64_t restore_id);
    // send everything pending now (shutdown); the host keeps the process alive
    // until the request finishes
    void flush();
}
