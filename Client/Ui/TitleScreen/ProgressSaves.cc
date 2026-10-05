#include <Client/Ui/TitleScreen/TitleScreen.hh>

#include <Client/Assets/Assets.hh>
#include <Client/Game.hh>

#include <format>

using namespace Ui;

ProgressPetalSlot::ProgressPetalSlot(uint32_t i, uint8_t p, float w) :
    Element(w, w, { .fill = 0x40000000, .round_radius = w / 20 }), idx(i), pos(p) {}

void ProgressPetalSlot::on_render(Renderer &ctx) {
    DEBUG_ONLY(assert(idx < Game::progress_saves.size());)
    uint8_t count = loadout_slots_at_level(score_to_level(Game::progress_saves[idx].score));
    PetalID::T id = Game::progress_saves[idx].loadout[Ui::UiLoadout::dynamic_to_static(pos, count)];
    if (pos < count || pos >= MAX_SLOT_COUNT)
        Element::on_render(ctx);
    if (id == PetalID::kNone) return;
    ctx.scale(width / 60);
    draw_loadout_background(ctx, id);
}

void ProgressPetalSlot::on_event(uint8_t event) {
    if (event == kFocusLost) {
        rendering_tooltip = 0;
        return;
    }
    DEBUG_ONLY(assert(idx < Game::progress_saves.size());)
    uint8_t count = loadout_slots_at_level(score_to_level(Game::progress_saves[idx].score));
    PetalID::T id = Game::progress_saves[idx].loadout[Ui::UiLoadout::dynamic_to_static(pos, count)];
    if (id == PetalID::kNone) return;
    rendering_tooltip = 1;
    tooltip = Ui::UiLoadout::petal_tooltips[id];
}

static Element *make_progress_card(uint32_t idx) {
    Element *elt = new Ui::VContainer({
        new Ui::HFlexContainer(
            new Ui::VContainer({
                new Ui::DynamicText(16, [=](){
                    if (idx >= Game::progress_saves.size()) return std::string();
                    return std::format("Score: {}", format_score(Game::progress_saves[idx].score));
                }, { .fill = 0xffffffff, .h_justify = Style::Left }),
                new Ui::DynamicText(16, [=](){
                    if (idx >= Game::progress_saves.size()) return std::string();
                    return std::format("Level: {}", score_to_level(Game::progress_saves[idx].score));
                }, { .fill = 0xffffffff, .h_justify = Style::Left }),
            }, 0, 3),
            new Ui::Button(100, 35,
                new Ui::StaticText(20, "Restore"),
                [=](Element *elt, uint8_t e){
                    if (e == Ui::kClick && Game::can_restore) {
                        DEBUG_ONLY(assert(idx < Game::progress_saves.size());)
                        Game::spawn_in(Game::progress_saves[idx].restore_id);
                    }
                },
                nullptr,
                { .fill = 0xff1dd129, .line_width = 5, .round_radius = 3, .should_render = [](){ return Game::can_restore; }, .no_animation = 1 }
            ), 10, 0
        ),
        new Ui::Element(0, 10),
        new Ui::HContainer(
            Ui::make_range(0, MAX_SLOT_COUNT, [=](uint32_t i){ return new ProgressPetalSlot(idx, i, 40); }),
            0, 5, { .h_justify = Style::Left }
        ),
        new Ui::Element(0, 5),
        new Ui::HContainer(
            Ui::make_range(MAX_SLOT_COUNT, 2 * MAX_SLOT_COUNT, [=](uint32_t i){ return new ProgressPetalSlot(idx, i, 35); }),
            0, 5, { .h_justify = Style::Left }
        )
    }, 10, 0, { .fill = 0x33000000, .round_radius = 6,
        .should_render = [=](){ return idx < Game::progress_saves.size(); },
        .v_justify = Style::Top, .no_animation = 1 });
    return elt;
}

DynamicProgressSaves::DynamicProgressSaves() : VContainer({}, 0, 10, { .no_animation = 1 }) {}

void DynamicProgressSaves::refactor() {
    while (children.size() < Game::progress_saves.size())
        add_child(make_progress_card(children.size()));
    VContainer::refactor();
    width = 555;
    height = std::max(height, 300.0f);
}

static Element *make_scroll() {
    Element *elt = new Ui::Choose(
        new Ui::Container({
            new Ui::VContainer({
                new Ui::StaticText(16, "Press G while staying still in-game to save your progress"),
                new Ui::StaticText(16, "Requires reaching Level 45+ or having a Legendary+ petal")
            }, 0, 5)
        }, 555, 300, { .no_animation = 1 }),
        new DynamicProgressSaves(),
        [](){ return Game::progress_saves.size() > 0 && Game::socket.ready; }
    );
    return new Ui::ScrollContainer(elt, 300);
}

Element *Ui::make_progress_saves() {
    Element *elt = new Ui::VContainer({
        new Ui::StaticText(25, "Progress Saves"),
        make_scroll()
    }, 15, 10, {
        .fill = 0xff5a9fdb,
        .line_width = 7,
        .round_radius = 3,
        .animate = [](Element *elt, Renderer &ctx){
            ctx.translate(0, (1 - elt->animation) * 2 * elt->height);
        },
        .should_render = [](){
            return Ui::panel_open == Panel::kSaves && Game::should_render_title_ui();
        },
        .h_justify = Style::Left,
        .v_justify = Style::Bottom
    });
    Ui::Panel::progress_saves = elt;
    return elt;
}
