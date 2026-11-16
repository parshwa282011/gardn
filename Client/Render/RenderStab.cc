#include <Client/Render/RenderEntity.hh>

#include <Client/Render/Renderer.hh>

#include <Client/Game.hh>

#include <Helpers/Math.hh>

#include <Shared/Entity.hh>

#include <cmath>

// devStabOSC: the circle where the TNT land, and what plays on the flower the TNT are
// coming for. Everything is drawn from the two numbers the server sends (the tenths of a
// second left, and how far the blast is), so nothing needs to be kept between frames.

static uint32_t argb(float alpha, uint32_t rgb) {
    return ((uint32_t) (fclamp(alpha, 0, 1) * 255) << 24) | (rgb & 0xffffff);
}

static bool is_stabbed(Entity const &ent) {
    return !ent.has_component(kMob) && (ent.get_stab_timer() > 0 || ent.get_stab_blast() > 0);
}

// 0 when the timer just started, 1 when the TNT land
static float urgency_of(Entity const &ent) {
    if (ent.get_stab_blast() > 0) return 1;
    return 1 - ent.get_stab_timer() / (float) (STAB_COUNTDOWN_SECONDS * 10);
}

// under the flowers: the circle follows the flower (it is drawn at its position), so the
// TNT always land on it
void render_stab_ground(Renderer &ctx, Entity const &ent) {
    if (!is_stabbed(ent)) return;
    float t = (float) (Game::timestamp / 1000.0);
    float urgency = urgency_of(ent);
    bool blasting = ent.get_stab_blast() > 0;
    float r = STAB_RADIUS;
    RenderContext c(&ctx);
    ctx.translate(ent.get_x(), ent.get_y());
    // the zone
    ctx.set_fill(argb(0.10f + 0.16f * urgency + (blasting ? 0.10f * (0.5f + 0.5f * sinf(t * 40)) : 0), 0xff2a55));
    ctx.begin_path();
    ctx.arc(0, 0, r);
    ctx.fill();
    // the ring, pulsing faster as the time runs out
    float pulse = 0.5f + 0.5f * sinf(t * (3 + 12 * urgency));
    ctx.set_stroke(argb(0.55f + 0.35f * pulse, blasting ? 0xffe9a0 : 0xff4d6d));
    ctx.set_line_width(5 + 3 * pulse);
    ctx.begin_path();
    ctx.arc(0, 0, r);
    ctx.stroke();
    // what is left of the timer, a bright arc over the ring
    if (!blasting) {
        float left = ent.get_stab_timer() / (float) (STAB_COUNTDOWN_SECONDS * 10);
        ctx.set_stroke(0xffffe9a0);
        ctx.set_line_width(7);
        ctx.round_line_cap();
        ctx.begin_path();
        ctx.partial_arc(0, 0, r, -M_PI / 2, -M_PI / 2 + 2 * M_PI * left, 0);
        ctx.stroke();
    }
    // two triangles turning against each other inside the ring: the seal
    ctx.set_line_width(3);
    ctx.round_line_join();
    for (uint32_t tri = 0; tri < 2; ++tri) {
        float spin = (tri == 0 ? 1 : -1) * t * (0.4f + 1.2f * urgency);
        ctx.set_stroke(argb(0.5f + 0.3f * urgency, tri == 0 ? 0xc77bff : 0xff7bd6));
        ctx.begin_path();
        for (uint32_t i = 0; i <= 3; ++i) {
            float a = spin + i * 2 * M_PI / 3 + tri * M_PI / 3;
            if (i == 0) ctx.move_to(r * 0.86f * cosf(a), r * 0.86f * sinf(a));
            else ctx.line_to(r * 0.86f * cosf(a), r * 0.86f * sinf(a));
        }
        ctx.stroke();
    }
    // runes on a second ring
    ctx.set_stroke(argb(0.6f, 0xffd1f0));
    ctx.set_line_width(2.5f);
    for (uint32_t i = 0; i < 12; ++i) {
        float a = -t * 0.5f + i * 2 * M_PI / 12;
        float rr = r * 0.66f;
        RenderContext rune(&ctx);
        ctx.translate(rr * cosf(a), rr * sinf(a));
        ctx.rotate(a + M_PI / 2);
        ctx.begin_path();
        switch (i % 3) {
            case 0:
                ctx.move_to(-6, 6); ctx.line_to(0, -8); ctx.line_to(6, 6); ctx.line_to(-6, 6);
                break;
            case 1:
                ctx.move_to(0, -8); ctx.line_to(0, 8); ctx.move_to(-6, -2); ctx.line_to(6, -2);
                break;
            default:
                ctx.move_to(-6, -6); ctx.line_to(6, 6); ctx.move_to(6, -6); ctx.line_to(-6, 6);
                break;
        }
        ctx.stroke();
    }
}

// over the flowers: sparks circling the flower, and when the time is up the TNT coming
// down in the circle and going off on it
void render_stab_effect(Renderer &ctx, Entity const &ent) {
    if (!is_stabbed(ent)) return;
    float t = (float) (Game::timestamp / 1000.0);
    float urgency = urgency_of(ent);
    float flower_r = ent.get_radius();
    RenderContext c(&ctx);
    ctx.translate(ent.get_x(), ent.get_y());
    // a halo around the flower
    ctx.set_fill(argb(0.18f + 0.22f * urgency, 0xff8bd6));
    ctx.begin_path();
    ctx.arc(0, 0, flower_r * (1.5f + 0.2f * sinf(t * 8)));
    ctx.fill();
    // sparks in a spiral
    for (uint32_t i = 0; i < 16; ++i) {
        float phase = fmodf(t * (0.6f + urgency) + i / 16.0f, 1.0f);
        float a = t * 2.5f + i * 2.4f;
        float rr = flower_r * (0.8f + 3.2f * phase);
        float size = (1 - phase) * (3 + 3 * urgency);
        ctx.set_fill(argb(1 - phase, i % 2 ? 0xffe9a0 : 0xd68bff));
        ctx.begin_path();
        ctx.arc(rr * cosf(a), rr * sinf(a), size);
        ctx.fill();
    }
    if (ent.get_stab_blast() == 0) return;
    float blast = ent.get_stab_blast() / 100.0f;
    // the TNT: each slot drops a crate at a place of the circle, which goes off when it
    // lands, and then takes the next place. (Only the ones in sight are drawn: the server
    // counts all 1000)
    uint32_t const slots = 44;
    for (uint32_t k = 0; k < slots; ++k) {
        float cycle_speed = 2.4f + (k % 5) * 0.3f;
        float x = t * cycle_speed + k * 0.37f;
        uint32_t cycle = (uint32_t) x;
        float phase = x - (float) cycle;
        SeedGenerator gen(k * 7919u + cycle * 104729u + 13u);
        float a = gen.next() * 2 * M_PI;
        float d = sqrtf(gen.next()) * STAB_RADIUS;
        float px = d * cosf(a), py = d * sinf(a);
        // the flower itself is hit now and then
        if (k % 4 == 0) { px *= 0.15f; py *= 0.15f; }
        RenderContext tnt(&ctx);
        ctx.translate(px, py);
        if (phase < 0.45f) {
            // falling: from far above, getting bigger
            float fall = phase / 0.45f;
            ctx.translate(0, -(1 - fall) * (1 - fall) * 420);
            ctx.scale(0.5f + 0.5f * fall);
            ctx.set_global_alpha(0.35f + 0.65f * fall);
            ctx.rotate(sinf(k + t * 6) * 0.3f);
            ctx.set_fill(0xffc9302c);
            ctx.set_stroke(0xff6e1512);
            ctx.set_line_width(2.5f);
            ctx.round_line_join();
            ctx.begin_path();
            ctx.round_rect(-9, -12, 18, 24, 2);
            ctx.fill();
            ctx.stroke();
            ctx.set_fill(0xffffe9a0);
            ctx.begin_path();
            ctx.rect(-9, -3, 18, 6);
            ctx.fill();
        } else {
            // going off: a fireball that grows and fades, and a ring
            float boom = (phase - 0.45f) / 0.55f;
            float rad = 18 + 52 * boom;
            ctx.set_fill(argb(0.85f * (1 - boom), 0xff8a1f));
            ctx.begin_path();
            ctx.arc(0, 0, rad);
            ctx.fill();
            ctx.set_fill(argb(0.9f * (1 - boom), 0xfff3a0));
            ctx.begin_path();
            ctx.arc(0, 0, rad * 0.55f);
            ctx.fill();
            ctx.set_stroke(argb(0.8f * (1 - boom), 0xffffff));
            ctx.set_line_width(3);
            ctx.begin_path();
            ctx.arc(0, 0, rad * 1.25f);
            ctx.stroke();
        }
    }
    // a pillar of light over the flower that dies down with the blast
    ctx.set_fill(argb(0.35f * (1 - blast) + 0.1f, 0xfff0ff));
    ctx.begin_path();
    ctx.arc(0, 0, flower_r * (1.2f + 1.5f * (1 - blast)));
    ctx.fill();
}
