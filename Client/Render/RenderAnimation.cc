#include <Client/Render/RenderEntity.hh>

#include <Client/Render/Renderer.hh>

#include <Client/Game.hh>

#include <Shared/Entity.hh>

void render_animation(Simulation *sim, Renderer &ctx, Entity const &ent) {
    switch (ent.get_anim_type()) {
        case AnimationType::kLightning: {
            if (sim->ent_exists(ent.get_seg_head())) break;
            ctx.set_global_alpha(1 - ent.deletion_animation);
            ctx.set_stroke(0xffffffff);
            ctx.set_line_width(2.65);
            ctx.begin_path();
            ctx.move_to(ent.get_x(), ent.get_y());
            Entity const *last = &ent;
            while (sim->ent_exists(last->get_seg_tail())) {
                Entity const &curr = sim->get_ent(last->get_seg_tail());
                SeedGenerator gen(curr.id.id * 1374686 + last->id.id * 23973 + 4829379);
                Vector delta(curr.get_x() - last->get_x(), curr.get_y() - last->get_y());
                float dist = delta.magnitude();
                uint32_t count = ceilf(dist / (50 + 100 * gen.next()));
                float radius = dist / count / 2;
                for (uint32_t i = 0; i < count; ++i) {
                    delta.set_magnitude(i * radius * 2 + radius);
                    Vector rand = Vector().unit_normal(2 * M_PI * gen.next()).set_magnitude(radius * gen.next());
                    Vector point = Vector(last->get_x(), last->get_y()) + delta + rand;
                    ctx.line_to(point.x, point.y);
                }
                ctx.line_to(curr.get_x(), curr.get_y());
                last = &curr;
            }
            ctx.stroke();
            break;
        }
        case AnimationType::kUranium:
            ctx.set_global_alpha(0.2 * (1 - ent.deletion_animation));
            ctx.set_fill(0xff63bf2e);
            ctx.begin_path();
            ctx.arc(ent.get_x(), ent.get_y(), ent.get_radius() * (1 - 0.25 * ent.deletion_animation));
            ctx.fill();
            break;
        case AnimationType::kExplosion: {
            // grows from the middle and fades (deletion_animation goes from 0 to 1)
            float t = fclamp(ent.deletion_animation, 0, 1);
            float r = ent.get_radius();
            ctx.set_global_alpha(1 - t);
            ctx.set_fill(0xffff8a1f);
            ctx.begin_path();
            ctx.arc(ent.get_x(), ent.get_y(), r * (0.35f + 0.65f * t));
            ctx.fill();
            ctx.set_fill(0xfffff3a0);
            ctx.begin_path();
            ctx.arc(ent.get_x(), ent.get_y(), r * (0.2f + 0.4f * t));
            ctx.fill();
            ctx.set_stroke(0xffffffff);
            ctx.set_line_width(3);
            ctx.begin_path();
            ctx.arc(ent.get_x(), ent.get_y(), r * (0.5f + 0.7f * t));
            ctx.stroke();
            break;
        }
        default:
            assert(!"Didn't cover animation render");
            break;
    }
}
