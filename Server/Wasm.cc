#ifdef WASM_SERVER
#include <Server/Client.hh>
#include <Server/Master.hh>
#include <Server/PetalTracker.hh>
#include <Server/Server.hh>

#include <Shared/Config.hh>
#include <Shared/Map.hh>

#include <iostream>
#include <string>
#include <unordered_map>

#include <emscripten.h>

std::unordered_map<int, WebSocket *> WS_MAP;

size_t const MAX_BUFFER_LEN = 1024;
static uint8_t INCOMING_BUFFER[MAX_BUFFER_LEN] = {0};

extern "C" {
    void on_connect(int ws_id) {
        std::printf("client connect: [%d]\n", ws_id);
        WebSocket *ws = new WebSocket(ws_id);
        WS_MAP.insert({ws_id, ws});
    }

    void on_disconnect(int ws_id, int reason) {
        auto iter = WS_MAP.find(ws_id);
        if (iter == WS_MAP.end()) {
            std::printf("unknown ws disconnect: [%d]", ws_id);
            return;
        }
        std::printf("client disconnect: [%d]\n", ws_id);
        Client::on_disconnect(iter->second, reason, {});
        delete iter->second;
        WS_MAP.erase(ws_id);
    }

    void on_message(int ws_id, uint32_t len) {
        auto iter = WS_MAP.find(ws_id);
        //WebSocket *ws = WS_MAP[ws_id];
        if (iter == WS_MAP.end()) return;
        std::string_view message(reinterpret_cast<char const *>(INCOMING_BUFFER), len);
        Client::on_message(iter->second, message, 0);
    }

    void tick() {
        Server::tick();
    }

    // The host asks the master about a player (Master::request_account); saves
    // come back one call each, then the verdict.
    std::unordered_map<int, std::vector<PlayerProgress>> LOADING_SAVES;

    void account_save(int ws_id, uint32_t hi, uint32_t lo, uint32_t score, uint8_t *loadout) {
        PlayerProgress progress = {};
        progress.restore_id = (static_cast<uint64_t>(hi) << 32) | lo;
        progress.score = score;
        for (uint32_t i = 0; i < 2 * MAX_SLOT_COUNT; ++i)
            progress.loadout[i] = loadout[i] < PetalID::kNumPetals ? loadout[i] : PetalID::kNone;
        LOADING_SAVES[ws_id].push_back(progress);
    }

    // status: 1 accepted, 2 expired, 3 master unreachable, anything else refused
    void account_loaded(int ws_id, int status) {
        std::vector<PlayerProgress> saves = std::move(LOADING_SAVES[ws_id]);
        LOADING_SAVES.erase(ws_id);
        auto iter = WS_MAP.find(ws_id);
        if (iter == WS_MAP.end()) return; // the player left while we waited
        iter->second->getUserData()->on_account(status, saves);
    }

    void sigusr2() {
        Server::is_draining = true;
    }

    bool restore_player(uint8_t gamemode, EntityID::hash_type hash, EntityID::id_type id, uint32_t score, PetalID::T *loadout_ids) {
        if (gamemode >= Gamemode::kNumGamemodes) return false;
        Simulation *sim = &Server::games[gamemode].simulation;
        if (id >= ENTITY_CAP) return false;
        EntityID player_id = EntityID(id, hash);
        if (!sim->ent_alive(player_id)) return false;
        Entity &player = sim->get_ent(player_id);
        if (!player.has_component(kFlower) || player.has_component(kMob)) return false;
        uint32_t loadout_count = loadout_slots_at_level(score_to_level(score));
        for (uint32_t i = 0; i < loadout_count + MAX_SLOT_COUNT; ++i)
            if (loadout_ids[i] >= PetalID::kNumPetals) return false;

        player.set_score(score);
        player.set_loadout_count(loadout_count);
        player.immunity_ticks = 3 * TPS;
        uint32_t difficulty = MAP_DATA[Map::get_zone_from_pos(player.get_x(), player.get_y())].difficulty;
        uint32_t power = Map::difficulty_at_level(score_to_level(score));
        if (difficulty < power) {
            ZoneDefinition const &zone = MAP_DATA[Map::get_suitable_difficulty_zone(power)];
            player.set_x(0.99 * zone.left + 0.01 * zone.right);
        }
        for (uint32_t i = 0; i < 2 * MAX_SLOT_COUNT; ++i) {
            PetalTracker::remove_petal(player.get_loadout_ids(i));
            player.set_loadout_ids(i, PetalID::kNone);
        }
        for (uint32_t i = 0; i < MAX_SLOT_COUNT; ++i) {
            LoadoutSlot &slot = player.loadout[i];
            slot.update_id(sim, PetalID::kNone);
        }
        for (uint32_t i = 0; i < loadout_count + MAX_SLOT_COUNT; ++i) {
            player.set_loadout_ids(i, loadout_ids[i]);
            PetalTracker::add_petal(loadout_ids[i]);
        }
        for (uint32_t i = 0; i < loadout_count; ++i) {
            LoadoutSlot &slot = player.loadout[i];
            slot.update_id(sim, loadout_ids[i]);
            slot.force_reload();
        }
        return true;
    }
}

WebSocketServer::WebSocketServer() {
    EM_ASM({
        const WSS = require("ws");
        const http = require("http");
        const fs = require("fs");
        const server = http.createServer(function(req, res) {
            let encodeType = "text/html";
            let file = "index.html";
            switch (req.url) {
                case "/":
                    break;
                case "/gardn-client.js":
                    encodeType = "application/javascript";
                    file = "gardn-client.js";
                    break;
                case "/gardn-client.wasm":
                    encodeType = "application/wasm";
                    file = "gardn-client.wasm";
                    break;
                default:
                    file = "";
                    break;
            }
            if (fs.existsSync(file)) {            
                res.writeHead(200, {"Content-Type": encodeType});
                res.end(fs.readFileSync(file));
                return;
            }
            res.writeHead(404, {"Content-Type": encodeType});
            res.end();
        });

        server.listen($0, function() {
            console.log("Server running at http://localhost:"+$0);
        });
        Module.server = server;
        
        const wss = new WSS.Server({ "server": server });
        Module.ws_connections = {};
        let curr_id = 0;
        wss.on("connection", function(ws, req) {
            const ws_id = curr_id;
            Module.ws_connections[ws_id] = ws;
            _on_connect(ws_id);
            curr_id = (curr_id + 1) | 0;
            ws.on("message", function(message) {
                let data = new Uint8Array(message);
                const len = data.length > $2 ? $2 : data.length;
                data = data.subarray(0, len);
                HEAPU8.set(data, $1);
                _on_message(ws_id, len);
            });
            ws.on("close", function(reason) {
                _on_disconnect(ws_id, reason);
                delete Module.ws_connections[ws_id];
            });
        })
    }, SERVER_PORT, INCOMING_BUFFER, MAX_BUFFER_LEN);
}

void Server::init() {
    Master::init();
    EM_ASM({
        globalThis.Module = Module;
        process.on("SIGUSR2", () => {
            console.log("closing...");
            require("inspector").close(); // might block
            _sigusr2();
            Module.master.flush(); // a draining server saves what it holds now
        });
        for (const signal of ["SIGINT", "SIGTERM"]) {
            process.on(signal, () => {
                console.log(signal + ": saving and exiting...");
                Module.master.flush().finally(() => process.exit(0));
            });
        }
        process.on("exit", () => {
            console.log("exiting...");
        });
    });

    for (GameInstance &game : Server::games) game.init();
    Server::run();
}

void Server::run() {
    EM_ASM({
        Module.tickInterval = setInterval(_tick, $0);
    }, 1000 / TPS);
}

void Server::stop() {
    Server::is_stopping = true;
    std::cout << "stopping...\n";
    Master::flush();
    EM_ASM({
        Module.server.close();
        for (const ws_id in Module.ws_connections) {
            const ws = Module.ws_connections[ws_id];
            ws.close(1001, "Shutting Down");
        }
        clearInterval(Module.tickInterval);
    });
}

void Client::send_packet(uint8_t const *packet, size_t size) {
    if (ws == nullptr) return;
    ws->send(packet, size);
}

WebSocket::WebSocket(int id) : ws_id(id) {
    client.ws = this;
}

void WebSocket::send(uint8_t const *packet, size_t size) {
    EM_ASM({
        if (!Module.ws_connections || !Module.ws_connections[$0]) return;
        const ws = Module.ws_connections[$0];
        ws.send(HEAPU8.subarray($1,$1+$2));
    }, ws_id, packet, size);
}

void WebSocket::end(int code, std::string const &message) {
    EM_ASM({
        if (!Module.ws_connections || !Module.ws_connections[$0]) return;
        const ws = Module.ws_connections[$0];
        ws.close($1, UTF8ToString($2));
    }, ws_id, code, message.c_str());
}

Client *WebSocket::getUserData() {
    return &client;
}

// ---------------------------------------------------------------- master

// The JS host for the master link. EM_JS (unlike EM_ASM) takes commas in the
// body, which this much JavaScript needs.
extern "C" {
EM_JS(void, master_init_js, (), {
        const fs = require("fs");
        /* .env next to the server (written by the deployment scripts); real */
        /* environment variables win */
        try {
            fs.readFileSync(".env", "utf8").split("\n").forEach(function(line) {
                const i = line.indexOf("=");
                if (i <= 0) return;
                const key = line.slice(0, i).trim();
                let value = line.slice(i + 1).trim();
                if (value.length > 1 && value[0] === String.fromCharCode(34) && value[value.length - 1] === value[0])
                    value = value.slice(1, -1);
                if (key && key[0] !== "#" && process.env[key] === undefined)
                    process.env[key] = value;
            });
        } catch (e) {}
        const M = Module.master = {
            url: (process.env["MASTER_URL"] || "http://127.0.0.1:55554"),
            secret: process.env["SERVER_SECRET"] || "",
            pending: new Map(), /* uuid -> { add: Map(id -> save), remove: Set(id), tries } */
            chain: Promise.resolve()
        };
        while (M.url.endsWith("/")) M.url = M.url.slice(0, -1);
        if (!M.secret)
            console.log("SERVER_SECRET is not set: accounts will not work");

        M.call = async function(path, body) {
            const r = await fetch(M.url + path, {
                method: "POST",
                headers: { "content-type": "application/json", "x-server-secret": M.secret },
                body: JSON.stringify(body)
            });
            let j = {};
            try { j = await r.json(); } catch (e) {}
            return [r.status, j];
        };
        M.entry = function(uuid) {
            let e = M.pending.get(uuid);
            if (!e) { e = { add: new Map(), remove: new Set(), tries: 0 }; M.pending.set(uuid, e); }
            return e;
        };
        /* put changes back after a failed send (newer changes win) */
        M.requeue = function(uuid, old) {
            if (++old.tries > 5) {
                console.log("giving up on saves of " + uuid);
                return;
            }
            const cur = M.entry(uuid);
            cur.tries = Math.max(cur.tries, old.tries);
            old.add.forEach(function(save, id) {
                if (!cur.remove.has(id) && !cur.add.has(id)) cur.add.set(id, save);
            });
            old.remove.forEach(function(id) { cur.remove.add(id); });
        };
        /* one flush at a time; `only` = a single account (used before loading it) */
        M.flush = function(only) {
            M.chain = M.chain.then(async function() {
                const taken = [];
                M.pending.forEach(function(e, uuid) { if (!only || uuid === only) taken.push([uuid, e]); });
                if (!taken.length) return;
                taken.forEach(function(t) { M.pending.delete(t[0]); });
                const changes = taken.map(function(t) {
                    return { "uuid": t[0], "add": Array.from(t[1].add.values()), "remove": Array.from(t[1].remove) };
                });
                let status = 0, j = {};
                try { [status, j] = await M.call("/srv/gardn/flush", { "changes": changes }); }
                catch (e) { console.log("master unreachable while saving:", e.message); }
                if (status !== 200) {
                    taken.forEach(function(t) { M.requeue(t[0], t[1]); });
                    return;
                }
                const failed = new Set(j["failed"] || []);
                taken.forEach(function(t) { if (failed.has(t[0])) M.requeue(t[0], t[1]); });
                console.log("saved progress of " + (taken.length - failed.size) + " account(s)");
            }).catch(function(e) { console.log("flush error:", e); });
            return M.chain;
        };
        M.load = async function(wsId, uuid, plp) {
            let status = 3, saves = [];
            try {
                await M.flush(uuid); /* unflushed saves of this account must be visible */
                const r = await M.call("/srv/gardn/load", { "uuid": uuid, "pl_p": plp });
                const st = r[0], j = r[1];
                if (st === 200 && j["ok"]) { status = 1; saves = j["saves"] || []; }
                else if (st === 401 && j["error"] === "expired") status = 2;
                else if (st === 401) status = 4;
                else status = 3; /* 403 = our own secret is wrong, 5xx = master trouble */
            } catch (e) { console.log("master unreachable:", e.message); }
            const ptr = _malloc(32);
            if (status === 1) {
                for (const s of saves) {
                    const id = String(s["save_id"] || "");
                    if (!/^[0-9a-f]{16}$/.test(id)) continue;
                    HEAPU8.fill(0, ptr, ptr + 32);
                    const loadout = s["loadout"] || [];
                    for (let i = 0; i < 24 && i < loadout.length; ++i) HEAPU8[ptr + i] = loadout[i] & 255;
                    _account_save(wsId, parseInt(id.slice(0, 8), 16) >>> 0, parseInt(id.slice(8), 16) >>> 0,
                                  (s["score"] || 0) >>> 0, ptr);
                }
            }
            _free(ptr);
            _account_loaded(wsId, status);
        };
        /* names that leave this process are quoted so the closure compiler keeps them */
        setInterval(function() { M.flush(); }, 60000).unref();
});
}

void Master::init() {
    master_init_js();
}

void Master::request_account(int ws_id, std::string const &uuid, std::string const &pl_p) {
    EM_ASM({
        Module.master.load($0, UTF8ToString($1), UTF8ToString($2));
    }, ws_id, uuid.c_str(), pl_p.c_str());
}

void Master::queue_add(std::string const &uuid, PlayerProgress const &progress) {
    EM_ASM({
        const id = ($1 >>> 0).toString(16).padStart(8, "0") + ($2 >>> 0).toString(16).padStart(8, "0");
        const e = Module.master.entry(UTF8ToString($0));
        e.remove.delete(id);
        e.add.set(id, { "save_id": id, "score": $3 >>> 0, "loadout": Array.from(HEAPU8.subarray($4, $4 + $5)) });
    }, uuid.c_str(), static_cast<uint32_t>(progress.restore_id >> 32), static_cast<uint32_t>(progress.restore_id & 0xffffffffu),
       progress.score, progress.loadout.data(), static_cast<int>(progress.loadout.size()));
}

void Master::queue_remove(std::string const &uuid, uint64_t restore_id) {
    EM_ASM({
        const id = ($1 >>> 0).toString(16).padStart(8, "0") + ($2 >>> 0).toString(16).padStart(8, "0");
        const e = Module.master.entry(UTF8ToString($0));
        e.add.delete(id);
        e.remove.add(id);
    }, uuid.c_str(), static_cast<uint32_t>(restore_id >> 32), static_cast<uint32_t>(restore_id & 0xffffffffu));
}

void Master::flush() {
    EM_ASM({ Module.master.flush(); });
}

WebSocketServer Server::server;
#endif
