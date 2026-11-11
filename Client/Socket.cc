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
// A browser can refuse to give the page its storage (Edge's tracking prevention,
// private windows, a page inside someone else's frame): reading it then throws.
// So the login is looked for in three places, none of which can break the page:
//   1. window.gardnLogin, handed over in the address by the Parteria page's Gardn
//      button and removed from the address at once (see public/index.html)
//   2. localStorage
//   3. sessionStorage (where index.html keeps the handoff for a reload)
static std::string local_storage(char const *key) {
    char buffer[256] = {0};
    EM_ASM({
        const key = UTF8ToString($0);
        let value = "";
        try {
            const h = window.gardnLogin;
            if (h) value = (key === "uuid" ? h.uuid : h.plp) || "";
        } catch (e) {}
        if (!value) try { value = window.localStorage[key] || ""; } catch (e) {}
        if (!value) try { value = window.sessionStorage["gardn_" + key] || ""; } catch (e) {}
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
            std::string uuid = local_storage("uuid");
            Game::has_login = !uuid.empty();
            w.write<std::string>(uuid);
            w.write<std::string>(local_storage("PL-P"));
            Game::socket.ready = 1; //force send
            Game::socket.send(w.packet, w.at - w.packet);
            Game::socket.ready = 0;
        } 
        else if (type == 2) {
            Game::on_game_screen = 0;
            Game::socket.ready = 0;
            std::printf("Disconnected [%d](%s)\n", len, reason);
            if (len == CloseReason::kExpired || len == CloseReason::kUnauthorized) {
                // an expired login, none, or an email that is not confirmed yet: the log in
                // page deals with all of them (relogin: forget the login this page was using)
                Game::disconnect_message = "Your login is not valid. Taking you to the log in page...";
                EM_ASM({
                    window.location.replace("/login?relogin=1&next=" + encodeURIComponent(window.location.pathname));
                });
            }
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