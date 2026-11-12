#include <Server/Process.hh>

#include <Server/EntityFunctions.hh>
#include <Shared/Simulation.hh>
#include <Shared/Entity.hh>

#include <cmath>
#include <iostream>

void tick_health_behavior(Simulation *sim, Entity &ent) {
    if (ent.poison_ticks > 0 && !ent.has_component(kPetal)) {
        ent.poison_ticks--;
        inflict_damage(sim, ent.poison_dealer, ent.id, ent.poison_inflicted, DamageType::kPoison);
        if (ent.poison_ticks % (TPS / 2) != 0) ent.set_damaged(0);
    } else {
        ent.poison_inflicted = 0;
        ent.poison_dealer = NULL_ENTITY;
    }
    if (get_sponge_period(sim, ent) > 0) {
        inflict_damage(sim, ent.last_damaged_by, ent.id, ent.delayed_damage[0], DamageType::kSponge);
        ent.delayed_damage.push_back(0);
    }
    if (ent.has_component(kFlower)) {
        float stored = 0;
        for (uint32_t i = 0; i < ent.delayed_damage.size(); ++i)
            stored += ent.delayed_damage[i];
        ent.set_sponge_stored((uint32_t) std::lround(stored));
    }
    if (ent.dandy_ticks > 0) --ent.dandy_ticks;
    ent.shield = fclamp(ent.shield - ent.shield / (25 * TPS), 0, ent.max_health);
    if (ent.max_health == 0) return;
    if (ent.health <= 0) sim->request_delete(ent.id);
    if (ent.has_component(kFlower)) {
        ent.set_health_ratio(ent.health / ent.max_health);
        ent.set_shield_ratio(ent.shield / ent.max_health);
    } else {
        ent.set_health_ratio(1);
        ent.set_shield_ratio(0);
    }
}