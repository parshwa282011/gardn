#include <Client/Render/RenderEntity.hh>

#include <Client/Game.hh>
#include <Client/Ui/Extern.hh>
#include <Client/Render/Renderer.hh>

#include <Shared/Entity.hh>

#include <algorithm>

// The health bar under a flower or a mob. It shows while the thing is hurt and fades when it is
// whole again; a boss keeps it on. The length follows the size of the thing but only between
// HEALTH_BAR_MIN_HALF and HEALTH_BAR_MAX_HALF (a boss may have a longer one), so the bar of a huge
// mob does not cross the screen and the bar of a tiny one can still be read.
static float const HEALTH_BAR_MIN_HALF = 22;
static float const HEALTH_BAR_MAX_HALF = 80;
static float const HEALTH_BAR_MAX_HALF_BOSS = 130;

void render_health(Renderer &ctx, Entity const &ent) {
    if (ent.has_component(kPetal)) return;
    bool is_mob = ent.has_component(kMob);
    // (one bar for a segmented mob: the head)
    if (is_mob && ent.has_component(kSegmented)) return;
    bool boss = is_mob && ent.get_boss();
    float opacity = boss ? 1.0f : ent.healthbar_opacity;
    if (opacity < 0.01) return;
    float reach = ent.get_radius() * 1.33;
    float w = std::min(std::max(reach, HEALTH_BAR_MIN_HALF), boss ? HEALTH_BAR_MAX_HALF_BOSS : HEALTH_BAR_MAX_HALF);
    // (the thickness: a boss has a thicker one)
    float back_width = boss ? 13 : 9, front_width = boss ? 10 : 7, shield_width = boss ? 7 : 5;
    float alpha = (1 - ent.deletion_animation) * opacity;
    ctx.set_global_alpha(alpha);
    ctx.scale(1 + 0.5 * ent.deletion_animation);
    // (just under the body, wherever the length stops growing)
    ctx.translate(-w, reach + 15);
    ctx.round_line_cap();
    ctx.set_stroke(0xff222222);
    ctx.set_line_width(back_width);
    ctx.begin_path();
    ctx.move_to(0, 0);
    ctx.line_to(2 * w, 0);
    ctx.stroke();
    ctx.set_global_alpha(alpha * std::min(ent.healthbar_lag * 20, 1.0f));
    ctx.set_stroke(0xffed2f31);
    ctx.set_line_width(front_width);
    ctx.begin_path();
    ctx.move_to(0, 0);
    ctx.line_to(2 * w * ent.healthbar_lag, 0);
    ctx.stroke();
    ctx.set_global_alpha(alpha * std::min(ent.get_health_ratio() * 20, 1.0f));
    ctx.set_stroke(0xff75dd34);
    ctx.set_line_width(front_width);
    ctx.begin_path();
    ctx.move_to(0, 0);
    ctx.line_to(2 * w * ent.get_health_ratio(), 0);
    ctx.stroke();
    ctx.set_global_alpha(alpha * std::min(ent.get_shield_ratio() * 20, 1.0f));
    ctx.set_stroke(0xffffffff);
    ctx.set_line_width(shield_width);
    ctx.begin_path();
    ctx.move_to(0, 0);
    ctx.line_to(2 * w * ent.get_shield_ratio(), 0);
    ctx.stroke();
    // the Yggdrasil of the Shiny Ladybug boss: a bar under its health, full and gold when it is loaded,
    // filling while it reloads
    if (boss && ent.get_mob_id() == MobID::kShinyLadybug) {
        float ready = ent.get_boss_reload() / 255.0f;
        ctx.translate(0, back_width + 8);
        ctx.set_global_alpha(alpha);
        ctx.set_stroke(0xff222222);
        ctx.set_line_width(9);
        ctx.begin_path();
        ctx.move_to(0, 0);
        ctx.line_to(2 * w, 0);
        ctx.stroke();
        ctx.set_stroke(ready >= 1 ? 0xffffd84a : 0xff6fb7ff);
        ctx.set_line_width(6);
        ctx.begin_path();
        ctx.move_to(0, 0);
        ctx.line_to(2 * w * std::max(ready, 0.02f), 0);
        ctx.stroke();
        ctx.translate(w, 13);
        ctx.draw_text(ready >= 1 ? "Ygg ready" : "Ygg reloading", { .size = 11 });
    }
}
