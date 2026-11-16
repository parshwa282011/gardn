#include <Client/Ui/InGame/GameInfo.hh>

#include <Client/Ui/Button.hh>
#include <Client/Ui/Container.hh>
#include <Client/Ui/DynamicText.hh>
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
        new Ui::StaticText(16, "Sandbox - nothing here is saved - press \\ for the menu", { .h_justify = Style::Left })
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

// devStabOSC: the player it hit has a timer on their screen, then the TNT come down on them
Element *Ui::make_stab_timer() {
    auto me = []() -> Entity const * {
        if (!Game::simulation.ent_alive(Game::player_id)) return nullptr;
        Entity const &player = Game::simulation.get_ent(Game::player_id);
        return player.has_component(kFlower) ? &player : nullptr;
    };
    Element *elt = new Ui::VContainer({
        new Ui::StaticText(16, "devStabOSC hit you", { .fill = 0xffff9fb4 }),
        new Ui::DynamicText(44, [me]() -> std::string {
            Entity const *player = me();
            if (player == nullptr) return "";
            if (player->get_stab_blast() > 0) return "IMPACT";
            uint32_t tenths = player->get_stab_timer();
            return std::to_string(tenths / 10) + "." + std::to_string(tenths % 10) + "s";
        }, { .fill = 0xffffe9a0 }),
        new Ui::StaticText(14, "1000 TNT are coming for you")
    }, 10, 4, {
        .fill = 0x80000000,
        .round_radius = 8,
        .animate = [](Element *elt, Renderer &ctx){
            ctx.set_global_alpha(elt->animation);
        },
        .should_render = [me]() {
            Entity const *player = me();
            return player != nullptr && (player->get_stab_timer() > 0 || player->get_stab_blast() > 0);
        }
    });
    elt->y = -160;
    return elt;
}
