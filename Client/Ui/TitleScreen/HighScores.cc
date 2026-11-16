#include <Client/Ui/TitleScreen/TitleScreen.hh>

#include <Client/Ui/Container.hh>
#include <Client/Ui/DynamicText.hh>
#include <Client/Ui/StaticText.hh>

#include <Client/Game.hh>

using namespace Ui;

static uint32_t const HIGH_SCORE_LINES = 5;

Element *Ui::make_high_scores() {
    std::vector<Element *> rows;
    rows.push_back(new Ui::StaticText(18, "High scores", { .fill = 0xffffe763, .h_justify = Style::Left }));
    for (uint32_t i = 0; i < HIGH_SCORE_LINES; ++i) {
        rows.push_back(new Ui::DynamicText(15, [i](){
            return i < Game::high_scores.size() ? Game::high_scores[i] : std::string{};
        }, {
            .fill = 0xffffffff,
            .should_render = [i](){ return i < Game::high_scores.size(); },
            .h_justify = Style::Left
        }));
    }
    Element *elt = new Ui::VContainer(rows, 10, 4, {
        .fill = 0x40000000,
        .round_radius = 5,
        .should_render = [](){
            return Game::should_render_title_ui() && !Game::high_scores.empty();
        },
        .h_justify = Style::Left,
        .v_justify = Style::Top
    });
    elt->x = 10;
    elt->y = 10;
    return elt;
}
