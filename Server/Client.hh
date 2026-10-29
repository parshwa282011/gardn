#pragma once

#include <Shared/Binary.hh>
#include <Shared/EntityDef.hh>

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
    uint8_t verifying = 0;      // waiting for the master to answer
    uint8_t account_counted = 0;
    uint64_t pending_recovery_id = 0;
    uint8_t pending_gamemode = 0;
    Client();
    void init(uint64_t, uint8_t);
    // status: 1 accepted, 2 PL-P expired, 3 master unreachable, else refused
    void on_account(uint8_t status, std::vector<struct PlayerProgress> &saves);
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