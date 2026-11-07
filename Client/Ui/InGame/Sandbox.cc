#include <Client/Ui/InGame/GameInfo.hh>

#include <Client/Ui/Button.hh>
#include <Client/Ui/Container.hh>
#include <Client/Ui/StaticText.hh>
#include <Client/Game.hh>

using namespace Ui;

// Top left, while playing in the sandbox: the X that leaves it, and a reminder that
// nothing in there is kept.
Element *Ui::make_sandbox_exit() {
    Element *elt = new Ui::HContainer({
        new Ui::Button(44, 44,
            new Ui::StaticText(24, "X"),
            [](Element *elt, uint8_t e){
                if (e == Ui::kClick) Game::switch_gamemode(Gamemode::kFFA);
            },
            nullptr,
            { .fill = 0xffec6869, .line_width = 5, .round_radius = 5 }
        ),
        new Ui::StaticText(16, "Sandbox - nothing here is saved", { .h_justify = Style::Left })
    }, 10, 10, {
        .should_render = [](){
            return Game::alive() && Game::simulation.arena_info.gamemode == Gamemode::kSandbox;
        },
        .h_justify = Style::Left,
        .v_justify = Style::Top,
        .no_animation = 1
    });
    elt->y = 55; // under the title
    return elt;
}
