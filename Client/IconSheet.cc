#include <Client/Game.hh>

#include <Client/Assets/Assets.hh>
#include <Client/Render/Renderer.hh>

#include <Shared/StaticData.hh>

#include <emscripten.h>

// The petal icons of the web pages (the petal stats page) are made with the game itself:
// open /gardn/?iconsheet and the game draws every petal as its loadout does (the
// group of petals, without the tile and the name: the page puts the rarity colour
// behind it), instead of playing. scripts/gen-icons.mjs (run by the build) takes the
// picture of that in headless Chrome and cuts every petal out as a png.
//
// ICON_COLUMNS tiles per row: the petal with id i is at column i % ICON_COLUMNS,
// row i / ICON_COLUMNS, each tile ICON_CELL units square
static uint32_t const ICON_COLUMNS = 12;
// (a petal that is a clump of several is wider than the petal itself: the tiles are big
// enough for the widest one with room to spare, so that no petal reaches into the next)
static float const ICON_CELL = 128;

bool Game::icon_sheet_mode() {
    static int requested = -1;
    if (requested < 0)
        requested = EM_ASM_INT({ return window.location.search.indexOf("iconsheet") >= 0 ? 1 : 0; });
    return requested == 1;
}

void Game::render_icon_sheet() {
    float dpr = EM_ASM_DOUBLE({ return window.devicePixelRatio || 1; });
    uint32_t rows = (PetalID::kNumPetals + ICON_COLUMNS - 1) / ICON_COLUMNS;
    // what scripts/gen-icons.mjs cuts out
    EM_ASM({
        // (the names in quotes: the build renames the others)
        window["iconSheetInfo"] = {};
        window["iconSheetInfo"]["columns"] = $0;
        window["iconSheetInfo"]["rows"] = $1;
        window["iconSheetInfo"]["cell"] = $2;
        window["iconSheetInfo"]["count"] = $3;
    }, ICON_COLUMNS, rows, ICON_CELL, (uint32_t) PetalID::kNumPetals);
    renderer.reset_transform();
    for (PetalID::T id = 1; id < PetalID::kNumPetals; ++id) {
        RenderContext c(&renderer);
        renderer.translate((id % ICON_COLUMNS + 0.5f) * ICON_CELL * dpr, (id / ICON_COLUMNS + 0.5f) * ICON_CELL * dpr);
        renderer.scale(dpr * 1.4f);
        if (PETAL_DATA[id].radius > 20) renderer.scale(20 / PETAL_DATA[id].radius);
        draw_static_petal(id, renderer);
    }
}
