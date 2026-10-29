#include <Client/Socket.hh>
#include <Client/DOM.hh>
#include <Client/Game.hh>

#include <Shared/Binary.hh>
#include <Shared/Config.hh>

#include <cstring>
#include <iostream>

#include <emscripten.h>

uint8_t INCOMING_PACKET[64 * 1024] = {0};
uint8_t OUTGOING_PACKET[1 * 1024] = {0};

// The rrolf login keeps "uuid" and "PL-P" in localStorage; gardn is served from
// the same origin, so it can show them to its server. Empty when logged out.
static std::string local_storage(char const *key) {
    char buffer[256] = {0};
    EM_ASM({
        const value = window.localStorage[UTF8ToString($0)] || "";
        const bytes = new TextEncoder().encode(value);
        const n = Math.min(bytes.length, $2 - 1);
        HEAPU8.set(bytes.subarray(0, n), $1);
        HEAPU8[$1 + n] = 0;
    }, key, buffer, sizeof buffer);
    return std::string(buffer);
}

extern "C" {
    void on_message(uint8_t type, uint32_t len, char *reason) {
        if (type == 0) {
            std::printf("Connected\n");
            Writer w(OUTGOING_PACKET);
            w.write<uint8_t>(Serverbound::kVerify);
            w.write<uint64_t>(VERSION_HASH);
            w.write<uint64_t>(Game::recovery_id);
            w.write<uint8_t>(Game::gamemode);
            w.write<std::string>(local_storage("uuid"));
            w.write<std::string>(local_storage("PL-P"));
            Game::socket.ready = 1; //force send
            Game::socket.send(w.packet, w.at - w.packet);
            Game::socket.ready = 0;
        } 
        else if (type == 2) {
            Game::on_game_screen = 0;
            Game::socket.ready = 0;
            std::printf("Disconnected [%d](%s)\n", len, reason);
            if (len == CloseReason::kExpired)
                Game::disconnect_message = "Your login expired. Open the main page and log in again.";
            else if (len == CloseReason::kUnauthorized)
                Game::disconnect_message = "Not logged in. Open the main page and log in.";
            else if (std::strlen(reason))
                Game::disconnect_message = std::format("Disconnected with code {} ({})", len, reason);
            else
                Game::disconnect_message = std::format("Disconnected with code {}", len);
            free(reason);
            if (len == CloseReason::kOutdated)
                DOM::update_client(500);
        }
        else if (type == 1) {
            Game::socket.ready = 1;
            Game::on_message(INCOMING_PACKET, len);
        }
    }
}

Socket::Socket() {}

void Socket::connect(std::string const url) {
    std::cout << "Connecting to " << url << '\n';
    EM_ASM({
        let string = UTF8ToString($1);
        function connect() {
            let socket = Module.socket = new WebSocket(string);
            socket.binaryType = "arraybuffer";
            socket.onopen = function() {
                _on_message(0, 0, 0);
            };
            socket.onclose = function(a) {
                _on_message(2, a.code, stringToNewUTF8(a.reason));
                setTimeout(connect, 1000);
            };
            socket.onmessage = function(event) {
                HEAPU8.set(new Uint8Array(event.data), $0);
                _on_message(1, event.data.byteLength, 0);
            };
        }
        setTimeout(connect, 1000);
    }, INCOMING_PACKET, url.c_str());
}

void Socket::send(uint8_t *ptr, uint32_t len) {
    if (ready == 0) return; 
    EM_ASM({
        if (Module.socket?.readyState == 1) {
            Module.socket.send(HEAPU8.subarray($0, $0+$1));
        }
    }, ptr, len);
}