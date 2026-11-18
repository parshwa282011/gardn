#include <Client/Render/RenderEntity.hh>

#include <Client/Assets/Assets.hh>
#include <Client/Render/Renderer.hh>

#include <Client/Game.hh>

#include <Shared/Entity.hh>

#include <cmath>
#include <iostream>

void render_flower(Renderer &ctx, Entity const &ent) {
    if (ent.has_component(kMob)) return;
    uint8_t face_flags = ent.get_face_flags();
    if (ent.deletion_animation > 0) BitMath::set(face_flags, FaceFlags::kDeadEyes);
    draw_static_flower(ctx, {
        .radius = ent.get_radius(),
        .eye_x = ent.eye_x,
        .eye_y = ent.eye_y,
        .mouth = ent.mouth,
        .cutter_angle = (float) (Game::timestamp / 200),
        .face_flags = face_flags,
        .equip_flags = ent.get_equip_flags(),
        .color = ent.get_color()
    });
    // proximity voice: a small badge when the mic is open, rings going out while talking
    if (ent.get_voice() != VoiceState::kOff) {
        float r = ent.get_radius();
        float t = (float) (Game::timestamp / 1000.0);
        ctx.set_fill(0xff3fbf6f);
        ctx.set_stroke(0xff1f7a43);
        ctx.set_line_width(2);
        ctx.begin_path();
        ctx.arc(r * 0.85f, -r * 0.85f, 6);
        ctx.fill();
        ctx.stroke();
        if (ent.get_voice() == VoiceState::kTalking) {
            for (uint32_t i = 0; i < 2; ++i) {
                float phase = fmodf(t * 1.6f + i * 0.5f, 1.0f);
                ctx.set_stroke(((uint32_t) ((1 - phase) * 150) << 24) | 0x3fbf6f);
                ctx.set_line_width(3);
                ctx.begin_path();
                ctx.arc(0, 0, r * (1.2f + 0.9f * phase));
                ctx.stroke();
            }
        }
    }
}