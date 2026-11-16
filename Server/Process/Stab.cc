#include <Server/Process.hh>

#include <Server/EntityFunctions.hh>

#include <Shared/Entity.hh>
#include <Shared/Simulation.hh>
#include <Shared/StaticData.hh>

#include <algorithm>
#include <cmath>

// devStabOSC hit a flower: the timer starts (a flower that already has one keeps it)
void apply_stab(Simulation *sim, Entity &petal, Entity &target) {
    if (!petal.has_component(kPetal) || petal.get_petal_id() != PetalID::kDevStabOSC) return;
    if (petal.pending_delete) return;
    if (!target.has_component(kFlower) || target.pending_delete || target.health <= 0) return;
    if (target.stab_ticks == 0 && target.stab_blast_ticks == 0) {
        target.stab_ticks = STAB_COUNTDOWN_SECONDS * TPS;
        target.stab_owner = petal.base_entity;
        target.set_stab_timer(STAB_COUNTDOWN_SECONDS * 10);
    }
    sim->request_delete(petal.id);
}

// `tnt` TNT hit the victim and everything else in the circle around it
static void _explode(Simulation *sim, Entity &victim, uint32_t tnt) {
    if (tnt == 0) return;
    float amount = tnt * STAB_TNT_DAMAGE;
    EntityID owner = victim.stab_owner;
    bool has_owner = sim->ent_alive(owner);
    EntityID owner_team = has_owner ? sim->get_ent(owner).get_team() : NULL_ENTITY;
    inflict_damage(sim, owner, victim.id, amount, DamageType::kContact);
    // the circle is centered on the victim wherever it goes, so whoever stands in it
    // (but not the owner or the owner's group) is hit as well
    sim->for_each<kHealth>([&](Simulation *, Entity &ent) {
        if (ent.id == victim.id || ent.pending_delete || ent.health <= 0) return;
        if (!ent.has_component(kPhysics)) return;
        if (!ent.has_component(kFlower) && !ent.has_component(kMob)) return;
        if (ent.has_component(kPetal)) return;
        if (has_owner && (ent.id == owner || ent.get_team() == owner_team)) return;
        float dx = ent.get_x() - victim.get_x();
        float dy = ent.get_y() - victim.get_y();
        float reach = STAB_RADIUS + ent.get_radius();
        if (dx * dx + dy * dy > reach * reach) return;
        inflict_damage(sim, owner, ent.id, amount, DamageType::kContact);
    });
}

void tick_stab_behavior(Simulation *sim, Entity &player) {
    if (player.pending_delete) return;
    if (player.stab_ticks > 0) {
        --player.stab_ticks;
        if (player.stab_ticks > 0) {
            // tenths of a second left, rounded up
            player.set_stab_timer((player.stab_ticks * 10 + TPS - 1) / TPS);
            return;
        }
        player.set_stab_timer(0);
        player.stab_blast_ticks = 1;
        player.stab_tnt_fired = 0;
    }
    if (player.stab_blast_ticks == 0) return;
    // the 1000 TNT come down evenly over the blast
    uint32_t const blast_ticks = STAB_BLAST_SECONDS * TPS;
    uint32_t due = (uint64_t) STAB_TNT_COUNT * player.stab_blast_ticks / blast_ticks;
    _explode(sim, player, due - player.stab_tnt_fired);
    player.stab_tnt_fired = due;
    player.set_stab_blast(std::max<uint32_t>(1, 100 * player.stab_blast_ticks / blast_ticks));
    if (player.stab_blast_ticks >= blast_ticks) {
        player.stab_blast_ticks = 0;
        player.stab_tnt_fired = 0;
        player.set_stab_blast(0);
        return;
    }
    ++player.stab_blast_ticks;
}
