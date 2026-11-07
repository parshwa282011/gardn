#include <Client/Ui/InGame/GameInfo.hh>

#include <Client/Ui/Container.hh>
#include <Client/Ui/StaticText.hh>
#include <Client/Game.hh>

#include <Shared/Map.hh>

using namespace Ui;

Minimap::Minimap(float w) : Element(w, w*ARENA_HEIGHT/ARENA_WIDTH, {}) {}

static uint8_t current_gamemode() {
    return Game::simulation.arena_info.gamemode;
}

void Minimap::refactor() {
    height = width * arena_height_of(current_gamemode()) / arena_width_of(current_gamemode());
    Element::refactor();
}

void Minimap::on_render(Renderer &ctx) {
    float const arena_w = arena_width_of(current_gamemode());
    ctx.set_line_width(7);
    ctx.set_stroke(0xff444444);
    ctx.stroke_rect(-width/2,-height/2,width,height);
    ctx.translate(-width/2,-height/2);
    ctx.scale(width/arena_w);
    if (current_gamemode() == Gamemode::kSandbox) {
        // the normal area and the PvP area
        ctx.set_fill(MAP_DATA[0].color);
        ctx.fill_rect(0, 0, SANDBOX_PVP_LEFT, SANDBOX_HEIGHT);
        ctx.set_fill(0xffc26a5a);
        ctx.fill_rect(SANDBOX_PVP_LEFT, 0, SANDBOX_WIDTH - SANDBOX_PVP_LEFT, SANDBOX_HEIGHT);
        for (uint8_t area = 0; area < 2; ++area) {
            RenderContext c(&ctx);
            ctx.translate(area == 0 ? SANDBOX_PVP_LEFT / 2.0f : (SANDBOX_PVP_LEFT + SANDBOX_WIDTH) / 2.0f, SANDBOX_HEIGHT / 2.0f);
            ctx.draw_text(area == 0 ? "Normal" : "PVP", { .size = SANDBOX_HEIGHT / 6.0f });
        }
    } else for (ZoneDefinition const &def : MAP_DATA) {
        ctx.set_fill(def.color);
        ctx.fill_rect(def.left, def.top, def.right - def.left, def.bottom - def.top);
        ctx.translate((def.left+def.right)/2,(def.top+def.bottom)/2);
        ctx.draw_text(def.name, { .size = (def.bottom-def.top)/2 });
        ctx.translate(-(def.left+def.right)/2,-(def.top+def.bottom)/2);
    }
    if (!Game::simulation.ent_exists(Game::camera_id)) return;
    Entity const &camera = Game::simulation.get_ent(Game::camera_id);
    // (the dots are the same size on the screen in every mode)
    float const dot = arena_w / 40;
    float const dot_line = arena_w / 120;
    Game::simulation.for_each<kDot>([&](Simulation *sim, Entity const &ent){
        if (ent.get_color() == ColorID::kGreen) return;
        if (ent.get_parent() == camera.get_player()) return;
        RenderContext c(&ctx);
        ctx.translate(ent.get_x(), ent.get_y());
        ctx.set_global_alpha(1 - ent.deletion_animation);
        ctx.scale(1 + 0.5 * ent.deletion_animation);
        ctx.set_fill(FLOWER_COLORS[ent.get_color()]);
        ctx.set_stroke(Renderer::HSV(FLOWER_COLORS[ent.get_color()], 0.8));
        ctx.set_line_width(dot_line);
        ctx.begin_path();
        ctx.arc(0, 0, dot);
        ctx.fill();
        ctx.stroke();
    });
    Game::simulation.for_each<kDot>([&](Simulation *sim, Entity const &ent){
        if (ent.get_color() != ColorID::kGreen) return;
        if (ent.get_parent() == camera.get_player()) return;
        RenderContext c(&ctx);
        ctx.translate(ent.get_x(), ent.get_y());
        ctx.set_global_alpha(1 - ent.deletion_animation);
        ctx.scale(1 + 0.5 * ent.deletion_animation);
        ctx.set_fill(FLOWER_COLORS[ent.get_color()]);
        ctx.set_stroke(Renderer::HSV(FLOWER_COLORS[ent.get_color()], 0.8));
        ctx.set_line_width(dot_line);
        ctx.begin_path();
        ctx.arc(0, 0, dot);
        ctx.fill();
        ctx.stroke();
    });
    ctx.set_fill(0xffffe763);
    ctx.set_stroke(Renderer::HSV(0xffffe763, 0.8));
    ctx.set_line_width(dot_line);
    ctx.begin_path();
    ctx.arc(camera.get_camera_x(), camera.get_camera_y(), dot);
    ctx.fill();
    ctx.stroke();
}

Element *Ui::make_minimap() {
    Element *elt = new Ui::VContainer({
        new Ui::StaticText(20, "Minimap"),
        new Ui::Minimap(300)
    }, 20, 10, { 
        .should_render = [](){ return Game::should_render_game_ui(); },
        .h_justify = Style::Right,
        .v_justify = Style::Bottom
    });
    elt->y = -65;
    return elt;
}