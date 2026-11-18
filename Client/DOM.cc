#include <Client/DOM.hh>

#include <Client/Input.hh>

#include <Helpers/UTF8.hh>

#include <emscripten.h>

void DOM::create_text_input(char const *name, uint32_t max_len) {
    EM_ASM(
    {
        const name = UTF8ToString($0);
        const elem = document.createElement('input');
        elem.id = name;
        elem.type = "text";
        elem.style.position = "absolute";
        elem.style["font-family"] = "Ubuntu";
        elem.style.display = 'none';
        elem.style["border-width"] = "0px";
        elem.style.background = "transparent";
        elem.style.border = "none";
        elem.style.outline = "none";
        elem.maxLength = ($1).toString();
        elem.setAttribute("spellcheck", "false");
        elem.addEventListener("keydown", e => {
            if (e.key === "Enter" && !e.repeat) {
                e.stopPropagation();
                _key_event(stringToNewUTF8(e.key), 0);
                elem.blur();
            }
        });
        elem.addEventListener("keyup", e => {
            if (e.key === "Enter" && !e.repeat) {
                e.stopPropagation();
                _key_event(stringToNewUTF8(e.key), 1);
            }
        });
        document.body.appendChild(elem);
    },
    name, max_len);
}

void DOM::element_show(char const *name)
{
    EM_ASM(
    {
        const name = UTF8ToString($0);
        const elem = document.getElementById(name);
        elem.style.display = 'block';
    },
    name);
}

void DOM::element_hide(char const *name)
{
    EM_ASM(
    {
        const name = UTF8ToString($0);
        const elem = document.getElementById(name);
        elem.style.display = 'none';
    },
    name);
}

void DOM::update_pos_and_dimension(char const *name, float x, float y, float w, float h)
{
    EM_ASM(
        {
            const name = UTF8ToString($0);
            const elem = document.getElementById(name);
            elem.style.left = ($1 - $3 / 2) / devicePixelRatio + "px";
            elem.style.top = ($2 - $4 / 2) / devicePixelRatio + "px";
            elem.style.width = ($3 / devicePixelRatio - $4 / devicePixelRatio * 0.2) + "px";
            elem.style.height = $4 / devicePixelRatio + "px";
            elem.style["font-size"] = $4 / devicePixelRatio * 0.66 + "px";
            elem.style["padding-left"] = $4 / devicePixelRatio * 0.1 + "px";
        },
        name, x, y, w, h);
}

std::string DOM::retrieve_text(char const *name, uint32_t max_length) {
    char *ptr = (char *) EM_ASM_PTR(
    {
        const name = UTF8ToString($0);
        const elem = document.getElementById(name);
        return stringToNewUTF8(elem.value);
    },
    name);
    std::string out{ptr};
    out = UTF8Parser::trunc_string(out, max_length);
    free(ptr);
    return out;
}

void DOM::update_text(char const *name, std::string const &contents, uint32_t max_length) {
    EM_ASM({
        const name = UTF8ToString($0);
        const elem = document.getElementById(name);
        elem.value = UTF8ToString($1).slice(0,$2);
    }, name, contents.c_str(), max_length);
}

void DOM::element_focus(char const *name) {
    EM_ASM({
        const name = UTF8ToString($0);
        const elem = document.getElementById(name);
        elem.focus();
    }, name);
}

void DOM::blur() {
    EM_ASM({
        if (document.activeElement)
            document.activeElement.blur();
    });
}

void DOM::open_page(char const *url) {
    EM_ASM({
        try {
            window.open(UTF8ToString($0));
        } catch(e) {}
    }, url);
}

void DOM::update_client(float delay) {
    EM_ASM({
        try { delete window.localStorage["version_hash"]; } catch (e) {}
        window.onbeforeunload = null;
        setTimeout(() => {
            window.location.reload();
        }, $0);
    }, delay);
}

void DOM::cache_client() {
    EM_ASM({
        try { window.localStorage["version_hash"] = $0.toString(); } catch (e) {}
        window.onerror = window.onunhandledrejection = null;
    }, VERSION_HASH);
}

void DOM::toggle_fullscreen() {
    EM_ASM({
        try {
            if (!document.fullscreenElement)
                document.documentElement.requestFullscreen();
            else
                document.exitFullscreen();
        } catch {}
    });
}

uint8_t DOM::text_input_focused() {
    return EM_ASM_INT({
        return (document.activeElement && document.activeElement.tagName == "INPUT") ? 1 : 0;
    });
}

void DOM::fetch_high_scores() {
    EM_ASM({
        try {
            if (window["gardnScoresBusy"]) return;
            window["gardnScoresBusy"] = true;
            // the master is api-game.<domain> when the game is on game.<domain> (like the
            // leaderboard page does it)
            const base = /^game\\./.test(location.hostname)
                ? location.protocol + "//" + location.host.replace(/^game\\./, "api-game.")
                : location.protocol + "//" + location.hostname + ":55554";
            fetch(base + "/leaderboard?game=gardn", { headers: { accept: "application/json" }, cache: "no-store" })
                .then(res => res.ok ? res.json() : Promise.reject())
                .then(data => {
                    // (quoted names: the closure compiler of the release build renames the
                    // others, and these are the names of the JSON of the master)
                    const lines = [];
                    const game = (data["games"] || []).find(g => g["id"] === "gardn");
                    const board = game && (game["boards"] || []).find(b => b["id"] === "gardn_best_score");
                    if (board)
                        for (const e of (board["entries"] || []).slice(0, 5))
                            lines.push(e["rank"] + ". " + e["name"] + " - " + Math.round(e["score"]));
                    window["gardnScores"] = lines.join("\\n");
                })
                .catch(() => {})
                .finally(() => { window["gardnScoresBusy"] = false; });
        } catch (e) {
            window["gardnScoresBusy"] = false;
        }
    });
}

std::string DOM::retrieve_high_scores() {
    char buffer[1024] = {0};
    EM_ASM({
        const bytes = new TextEncoder().encode(window["gardnScores"] || "");
        const n = Math.min(bytes.length, $1 - 1);
        HEAPU8.set(bytes.subarray(0, n), $0);
        HEAPU8[$0 + n] = 0;
    }, buffer, sizeof buffer);
    return std::string(buffer);
}

// ---- proximity voice chat. The script itself is in index.html (window.gardnVoice, plain
// JavaScript the closure compiler does not touch); these only pass things across.
// (EM_JS: the body has commas, which EM_ASM cannot take)
extern "C" {
EM_JS(void, voice_init_js, (), {
    window["gardnVoiceBridge"] = {
        "signal": function(to, text) { _voice_signal(to, stringToNewUTF8(text)); },
        "state": function(state) { _voice_state(state); },
        "notice": function(text) { _voice_notice(stringToNewUTF8(text)); }
    };
});
}

void DOM::voice_init() {
    voice_init_js();
}

void DOM::voice_toggle() {
    EM_ASM({
        const v = window["gardnVoice"];
        if (v) v["toggle"]();
    });
}

void DOM::voice_update(uint32_t mine, std::string const &peers) {
    EM_ASM({
        const v = window["gardnVoice"];
        if (v) v["update"]($0, UTF8ToString($1));
    }, mine, peers.c_str());
}

void DOM::voice_signal_in(uint32_t from, std::string const &text) {
    EM_ASM({
        const v = window["gardnVoice"];
        if (v) v["signal"]($0, UTF8ToString($1));
    }, from, text.c_str());
}
