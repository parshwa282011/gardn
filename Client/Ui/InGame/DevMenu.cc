#include <Client/Ui/InGame/DevMenu.hh>

#include <Client/Assets/Assets.hh>
#include <Client/DOM.hh>
#include <Client/Game.hh>
#include <Client/Input.hh>
#include <Client/StaticData.hh>
#include <Client/Ui/Button.hh>
#include <Client/Ui/Container.hh>
#include <Client/Ui/DynamicText.hh>
#include <Client/Ui/Extern.hh>
#include <Client/Ui/ScrollContainer.hh>
#include <Client/Ui/StaticText.hh>
#include <Client/Ui/TextInput.hh>
#include <Client/Render/Renderer.hh>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace Ui;

static Element *menu_root = nullptr;

// whether the name has the search text in it, ignoring the case
static bool matches_filter(char const *name, std::string const &filter) {
    if (filter.empty()) return true;
    std::string lname, lfilter;
    for (char const *c = name; *c != 0; ++c)
        lname.push_back((char) std::tolower((unsigned char) *c));
    for (char c : filter)
        lfilter.push_back((char) std::tolower((unsigned char) c));
    return lname.find(lfilter) != std::string::npos;
}

// a number typed in a field, ok = 0 when it is not one
static uint32_t parse_number(std::string const &text, uint8_t *ok) {
    *ok = 0;
    if (text.empty() || text.size() > 10) return 0;
    for (char c : text)
        if (c < '0' || c > '9') return 0;
    *ok = 1;
    return (uint32_t) std::strtoul(text.c_str(), nullptr, 10);
}

static void draw_row_background(Element &row, Renderer &ctx) {
    row.Element::on_render(ctx);
    if (row.focus_state == kMouseHover || row.focus_state == kMouseDown) {
        ctx.set_fill(0xa05a9fdb);
        ctx.begin_path();
        ctx.round_rect(-row.width / 2, -row.height / 2, row.width, row.height, row.style.round_radius);
        ctx.fill();
    }
}

DevPetalRow::DevPetalRow(PetalID::T petal_id) :
    Element(206, 30, { .fill = 0x605a9fdb, .round_radius = 3, .no_animation = 1 }), id(petal_id)
{
    style.should_render = [petal_id](){
        return matches_filter(PETAL_DATA[petal_id].name, Game::dev_search_text);
    };
}

void DevPetalRow::on_render(Renderer &ctx) {
    draw_row_background(*this, ctx);
    {
        RenderContext c(&ctx);
        ctx.translate(-width / 2 + height / 2 + 4, 0);
        ctx.scale((height - 8) / 60);
        draw_loadout_background(ctx, id);
    }
    {
        RenderContext c(&ctx);
        char const *name = PETAL_DATA[id].name;
        ctx.translate(-width / 2 + height + 10 + Renderer::get_ascii_text_size(name) * 13 / 2, 0);
        ctx.draw_text(name, { .fill = RARITY_COLORS[PETAL_DATA[id].rarity], .size = 13, .stroke_scale = 0.12 });
    }
}

void DevPetalRow::on_event(uint8_t event) {
    if (event == kClick) Game::send_dev_give_petal(id);
}

DevMobRow::DevMobRow(MobID::T mob_id) :
    Element(206, 30, { .fill = 0x605a9fdb, .round_radius = 3, .no_animation = 1 }), id(mob_id)
{
    style.should_render = [mob_id](){
        return matches_filter(MOB_DATA[mob_id].name, Game::dev_search_text);
    };
}

void DevMobRow::on_render(Renderer &ctx) {
    draw_row_background(*this, ctx);
    {
        // the mob as the mob gallery draws it, small, in the colour of its rarity
        RenderContext c(&ctx);
        float box = height - 6;
        ctx.translate(-width / 2 + height / 2 + 2, 0);
        ctx.set_fill(0x40000000);
        ctx.begin_path();
        ctx.round_rect(-box / 2, -box / 2, box, box, 3);
        ctx.fill();
        ctx.clip();
        MobData const &data = MOB_DATA[id];
        float radius = (data.radius.upper + data.radius.lower) / 2;
        if (id != MobID::kDigger) ctx.rotate(-3 * M_PI / 4);
        if (id == MobID::kBeetle || id == MobID::kMassiveBeetle) ctx.translate(-5, 0);
        // the whole body fits in the box (a mob with segments shows its head)
        ctx.scale(box * 0.42f / radius);
        draw_static_mob(id, ctx, { .radius = radius, .flower_attrs = { .color = ColorID::kGray } });
    }
    RenderContext c(&ctx);
    char const *name = MOB_DATA[id].name;
    ctx.translate(-width / 2 + height + 10 + Renderer::get_ascii_text_size(name) * 13 / 2, 0);
    ctx.draw_text(name, { .fill = RARITY_COLORS[MOB_DATA[id].rarity], .size = 13, .stroke_scale = 0.12 });
}

void DevMobRow::on_event(uint8_t event) {
    if (event == kClick) Game::send_dev_spawn_mob(id);
}

// the list of the tab: sorted by rarity, then by name. Unique petals and mobs are left
// out (there is only one of them for all the games)
static Element *make_list(bool petals) {
    // (the rows go in the constructor: it is what lines them up from the top)
    std::vector<Element *> rows;
    if (petals) {
        std::vector<PetalID::T> ids;
        for (PetalID::T i = PetalID::kBasic; i < PetalID::kNumPetals; ++i)
            if (PETAL_DATA[i].rarity != RarityID::kUnique) ids.push_back(i);
        std::sort(ids.begin(), ids.end(), [](PetalID::T a, PetalID::T b){
            if (PETAL_DATA[a].rarity != PETAL_DATA[b].rarity) return PETAL_DATA[a].rarity < PETAL_DATA[b].rarity;
            return strcmp(PETAL_DATA[a].name, PETAL_DATA[b].name) < 0;
        });
        for (PetalID::T id : ids) rows.push_back(new Ui::DevPetalRow(id));
    } else {
        std::vector<MobID::T> ids;
        for (MobID::T i = 0; i < MobID::kNumMobs; ++i)
            if (MOB_DATA[i].rarity != RarityID::kUnique) ids.push_back(i);
        std::sort(ids.begin(), ids.end(), [](MobID::T a, MobID::T b){
            if (MOB_DATA[a].rarity != MOB_DATA[b].rarity) return MOB_DATA[a].rarity < MOB_DATA[b].rarity;
            return strcmp(MOB_DATA[a].name, MOB_DATA[b].name) < 0;
        });
        for (MobID::T id : ids) rows.push_back(new Ui::DevMobRow(id));
    }
    Element *list = new Ui::VContainer(rows, 2, 2, {});
    Element *scroll = new Ui::ScrollContainer(list, 250);
    scroll->style.should_render = [petals](){ return Game::dev_tab == (petals ? 0 : 1); };
    return scroll;
}

static Element *make_tab_buttons() {
    return new Ui::HContainer({
        new Ui::Button(100, 28,
            new Ui::StaticText(14, "Petals"),
            [](Element *, uint8_t e){ if (e == Ui::kClick) Game::dev_tab = 0; },
            [](){ return Game::dev_tab == 0; },
            { .fill = 0xff5a9fdb, .line_width = 3, .round_radius = 3 }
        ),
        new Ui::Button(100, 28,
            new Ui::StaticText(14, "Mobs"),
            [](Element *, uint8_t e){ if (e == Ui::kClick) Game::dev_tab = 1; },
            [](){ return Game::dev_tab == 1; },
            { .fill = 0xff5a9fdb, .line_width = 3, .round_radius = 3 }
        )
    }, 0, 6, {});
}

static Element *make_number_row(std::string &text, uint32_t max_len, char const *label, void (*send)(uint32_t), uint32_t cap) {
    return new Ui::HContainer({
        new Ui::TextInput(text, 118, 25, max_len, {}),
        new Ui::Button(80, 25,
            new Ui::StaticText(13, label),
            [&text, send, cap](Element *, uint8_t e){
                if (e != Ui::kClick) return;
                uint8_t ok = 0;
                uint32_t value = parse_number(text, &ok);
                if (ok) send(std::min(std::max<uint32_t>(value, 1), cap));
            },
            nullptr,
            { .fill = 0xff1dd129, .line_width = 3, .round_radius = 3 }
        )
    }, 0, 8, {});
}

Element *Ui::make_dev_menu() {
    Element *elt = new Ui::VContainer({
        new Ui::StaticText(22, "Developer Menu"),
        make_tab_buttons(),
        make_number_row(Game::dev_level_text, 3, "Set level", Game::send_dev_set_level, MAX_LEVEL),
        make_number_row(Game::dev_score_text, 10, "Set score", Game::send_dev_set_score, 0xffffffff),
        new Ui::TextInput(Game::dev_search_text, 206, 25, 20, { .line_width = 3, .round_radius = 3 }),
        make_list(true),
        make_list(false),
        new Ui::HContainer({
            new Ui::Button(100, 28,
                new Ui::StaticText(13, "Clear mobs"),
                [](Element *, uint8_t e){ if (e == Ui::kClick) Game::send_dev_clear_mobs(); },
                nullptr,
                { .fill = 0xff5a9fdb, .line_width = 3, .round_radius = 3 }
            ),
            new Ui::Button(100, 28,
                new Ui::DynamicText(13, [](){
                    if (Game::simulation.ent_exists(Game::camera_id) && Game::simulation.get_ent(Game::camera_id).get_dev_invincible())
                        return std::string{"Invincible: ON"};
                    return std::string{"Invincible: OFF"};
                }),
                [](Element *, uint8_t e){ if (e == Ui::kClick) Game::send_dev_toggle_invincible(); },
                [](){
                    return Game::simulation.ent_exists(Game::camera_id) && Game::simulation.get_ent(Game::camera_id).get_dev_invincible() != 0;
                },
                { .fill = 0xffec6869, .line_width = 3, .round_radius = 3 }
            )
        }, 0, 6, {}),
        new Ui::StaticText(11, "Sandbox only, nothing is saved - \\ or ESC closes")
    }, 12, 8, {
        .fill = 0xd0222222,
        .line_width = 6,
        .round_radius = 5,
        .animate = [](Element *elt, Renderer &ctx){ ctx.scale((float) elt->animation); },
        .should_render = [](){
            return Game::show_dev_menu && Game::socket.ready && Game::simulation_ready
                && Game::simulation.ent_exists(Game::camera_id);
        }
    });
    elt->style.h_justify = Ui::Style::Left;
    elt->style.v_justify = Ui::Style::Middle;
    elt->x = 20;
    menu_root = elt;
    return elt;
}

bool Ui::dev_menu_contains_mouse() {
    if (menu_root == nullptr || !menu_root->visible) return false;
    return std::abs(Input::mouse_x - menu_root->screen_x) < menu_root->width * Ui::scale / 2
        && std::abs(Input::mouse_y - menu_root->screen_y) < menu_root->height * Ui::scale / 2;
}
