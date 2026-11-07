#pragma once

#include <Shared/Binary.hh>
#include <Shared/EntityDef.hh>

#include <array>
#include <cstdint>
#include <set>
#include <string>

#ifdef WASM_SERVER
class WebSocket;
#else
#include <App.h>
class Client;
typedef uWS::WebSocket<false, true, Client> WebSocket;
#endif

class GameInstance;

class Client {
public:
    GameInstance *game;
    EntityID camera;
    std::set<EntityID> in_view;
    WebSocket *ws;
    uint8_t verified = 0;
    uint8_t seen_arena = 0;
    // account (empty uuid = anonymous: can play, cannot save progress)
    std::string account_uuid;
    uint8_t summon_fails = 0;   // wrong /summon passwords; the command stops answering after a few
    uint8_t verifying = 0;      // waiting for the master to answer
    uint64_t pending_recovery_id = 0;
    uint8_t pending_gamemode = 0;
    // a spawn that is waiting for the master to hand over the save it restores
    uint8_t restoring = 0;
    uint8_t restoring_dev = 0;
    std::string restoring_name;
    uint64_t restoring_id = 0;
    // what the player had before going into the sandbox (nothing that happens in the
    // sandbox is kept): it is put back when they leave it
    bool sandbox_snapshot = false;
    uint32_t sandbox_respawn_level = 1;
    std::array<PetalID::T, 2 * MAX_SLOT_COUNT> sandbox_inventory = {};
    Client();
    // `saves`: what the master has for the account, shown as progress entities
    void init(uint64_t, uint8_t, std::vector<struct PlayerProgress> const & = {});
    // status: 1 accepted, 2 PL-P expired, 3 master unreachable, else refused
    void on_account(uint8_t status, std::vector<struct PlayerProgress> &saves);
    // the answer to Master::request_restore. status: 1 here it is (`progress`),
    // 4 the save is gone, anything else: the master could not be asked
    void on_restore(uint8_t status, struct PlayerProgress const *progress);
    // puts a player into the world (name, dev flag, the save it restores or null)
    void spawn(std::string const &name, uint8_t dev, struct PlayerProgress const *progress);
    void move(GameInstance *);
    void remove();
    void disconnect(int = CloseReason::kProtocol, std::string const & = "Protocol Error");
    uint8_t alive();

    void send_packet(uint8_t const *, size_t);
    //takes in a bool expr
    //if true, packet reading should be terminated
    //optionally, the client can also be disconnected
    bool check_invalid(bool);
    static void on_message(WebSocket *, std::string_view, uint64_t);
    static void on_disconnect(WebSocket *, int, std::string_view);
};

#ifdef WASM_SERVER
class WebSocket {
    int ws_id;
public:
    Client client;
    WebSocket(int);
    Client *getUserData();
    int id() const { return ws_id; }
    void send(uint8_t const *, size_t);
    void end(int, std::string const &);
};
#endif