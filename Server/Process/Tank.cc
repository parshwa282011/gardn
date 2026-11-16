#include <Server/Process.hh>

#include <Server/EntityFunctions.hh>

#include <Shared/Entity.hh>
#include <Shared/Simulation.hh>
#include <Shared/StaticData.hh>

#include <cmath>

static bool _is_shell(Entity const &ent) {
    return ent.has_component(kPetal) && BitMath::at(ent.get_petal_flags(), PetalFlags::kTankShell);
}

// The shell goes off where it is: it is gone, and everything of another team inside the
// circle takes the damage (once). The circle is shown by an animation entity.
static void _explode(Simulation *sim, Entity &shell) {
    if (shell.pending_delete) return;
    EntityID shooter = shell.base_entity;
    float x = shell.get_x(), y = shell.get_y();
    sim->request_delete(shell.id);
    sim->for_each<kHealth>([&](Simulation *, Entity &ent) {
        if (ent.pending_delete || ent.health <= 0 || ent.id == shell.id) return;
        if (!ent.has_component(kPhysics)) return;
        if (!ent.has_component(kFlower) && !ent.has_component(kMob)) return;
        if (ent.get_team() == shell.get_team()) return;
        float dx = ent.get_x() - x, dy = ent.get_y() - y;
        float reach = TANK_EXPLOSION_RADIUS + ent.get_radius();
        if (dx * dx + dy * dy > reach * reach) return;
        inflict_damage(sim, shooter, ent.id, TANK_EXPLOSION_DAMAGE, DamageType::kContact);
    });
    Entity &anim = sim->alloc_ent();
    anim.add_component(kPhysics);
    anim.set_x(x);
    anim.set_y(y);
    anim.set_radius(TANK_EXPLOSION_RADIUS);
    anim.add_component(kAnimation);
    anim.set_anim_type(AnimationType::kExplosion);
    sim->spatial_hash.insert(anim);
    sim->request_delete(anim.id);
}

// a shell touched `other`: if it is a shell it goes off
void tank_shell_hit(Simulation *sim, Entity &shell) {
    if (_is_shell(shell)) _explode(sim, shell);
}

// every tick of a shell: a wall sets it off too
void tick_tank_shell(Simulation *sim, Entity &shell) {
    if (!_is_shell(shell) || shell.pending_delete) return;
    float width = arena_width_of(sim->arena_info.gamemode);
    float height = arena_height_of(sim->arena_info.gamemode);
    float r = shell.get_radius();
    if (shell.get_x() - r <= 0 || shell.get_x() + r >= width ||
        shell.get_y() - r <= 0 || shell.get_y() + r >= height)
        _explode(sim, shell);
}
