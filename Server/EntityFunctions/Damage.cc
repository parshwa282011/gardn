#include <Server/EntityFunctions.hh>

#include <Server/Spawn.hh>
#include <Shared/Entity.hh>
#include <Shared/Simulation.hh>

#include <cmath>

static bool _yggdrasil_revival_clause(Simulation *sim, Entity &player) {
    if (BitMath::at(player.flags, EntityFlags::kZombie)) return false;
    for (uint32_t i = 0; i < player.get_loadout_count(); ++i) {
        LoadoutSlot &slot = player.loadout[i];
        if (slot.get_petal_id() != PetalID::kYggdrasil) continue;
        for (uint32_t j = 0; j < slot.size(); ++j) {
            LoadoutPetal &petal_slot = slot.petals[j];
            if (sim->ent_alive(petal_slot.ent_id)) {
                sim->request_delete(petal_slot.ent_id);
                return true;
            }
        }
    }
    return false;
}

game_tick_t get_sponge_period(Simulation *sim, Entity &player) {
    if (!player.has_component(kFlower)) return 0;
    for (uint32_t i = 0; i < player.get_loadout_count(); ++i) {
        LoadoutSlot &slot = player.loadout[i];
        if (slot.get_petal_id() != PetalID::kSponge) continue;
        for (uint32_t j = 0; j < slot.size(); ++j) {
            LoadoutPetal &petal_slot = slot.petals[j];
            if (sim->ent_alive(petal_slot.ent_id))
                return PETAL_DATA[slot.get_petal_id()].attributes.period * TPS;
        }
    }
    return 0;
}

void inflict_damage(Simulation *sim, EntityID const atk_id, EntityID const def_id, float amt, uint8_t type) {
    if (amt <= 0) return;
    if (!sim->ent_alive(def_id)) return;
    Entity &defender = sim->get_ent(def_id);
    if (!defender.has_component(kHealth)) return;
    DEBUG_ONLY(assert(!defender.pending_delete);)
    DEBUG_ONLY(assert(defender.has_component(kHealth));)
    if (type != DamageType::kLightning && sim->ent_alive(atk_id)) {
        Entity &attacker = sim->get_ent(atk_id);
        if (attacker.has_component(kPetal) && attacker.get_petal_id() == PetalID::kLightning) {
            inflict_lightning(sim, attacker, defender, amt, PETAL_DATA[attacker.get_petal_id()].attributes.bounces);
            sim->request_delete(atk_id);
            return;
        }
    }
    if (defender.immunity_ticks > 0) return;
    if (type == DamageType::kContact) amt -= defender.armor;
    else if (type == DamageType::kPoison) amt -= defender.poison_armor;
    if (amt <= 0) return;
    //if (amt <= defender.armor) return;
    float damage_dealt = std::min(amt, defender.health + defender.shield);
    if (type != DamageType::kPassive) {
        if (type != DamageType::kSponge) {
            defender.set_damaged(1);
            if (type != DamageType::kReflect) {
                game_tick_t period = get_sponge_period(sim, defender);
                if (period > 0) {
                    DEBUG_ONLY(assert(period <= MAX_SPONGE_PERIOD);)
                    float dmg = amt / period;
                    for (uint32_t i = 0; i < period; ++i)
                        defender.delayed_damage[i] += dmg;
                    amt = 0;
                }
            }
        }
        float old_shield = defender.shield;
        defender.shield = fclamp(defender.shield - amt, 0, defender.shield);
        amt -= old_shield - defender.shield;
    }
    float old_health = defender.health;
    defender.health = fclamp(defender.health - amt, 0, defender.health);  
    amt -= old_health - defender.health;
    //ant hole spawns
    //floor start, ceil end
    if (defender.has_component(kMob) && defender.get_mob_id() == MobID::kAntHole) {
        uint32_t const num_waves = ANTHOLE_SPAWNS.size() - 1;
        uint32_t start = ceilf((defender.max_health - old_health) / defender.max_health * num_waves);
        uint32_t end = ceilf((defender.max_health - defender.health) / defender.max_health * num_waves);
        if (defender.health <= 0) end = num_waves + 1;
        for (uint32_t i = start; i < end; ++i) {
            for (MobID::T mob_id : ANTHOLE_SPAWNS[i]) {
                Entity &child = alloc_mob(
                    sim, mob_id, 
                    defender.get_x(), defender.get_y(), 
                    defender.get_team(), [](Entity &mob) {
                    mob.score_reward = MOB_DATA[mob.get_mob_id()].xp;
                    BitMath::set(mob.flags, EntityFlags::kHasCulling);
                });
                child.set_parent(defender.id);
                child.target = defender.target;
            }
        }
    }
    if (defender.has_component(kMob) && defender.get_mob_id() == MobID::kAntBurrow) {
        if (!defender.activated) {
            defender.activated = 1;
            defender.pending_spawn_count = FIRE_ANT_COUNT;
        }
    }
    // yggdrasil revive clause
    if (defender.health == 0 && defender.has_component(kFlower)) {
        if (_yggdrasil_revival_clause(sim, defender)) {
            defender.set_revived(1);
            defender.health = defender.max_health;
            defender.poison_ticks = 0;
            defender.slow_ticks = 0;
            defender.dandy_ticks = 0;
            defender.honey_ticks = 0;
            defender.immunity_ticks = 1.0 * TPS;
            for (uint32_t i = 0; i < MAX_SPONGE_PERIOD; ++i)
                defender.delayed_damage[i] = 0;
        } else
            sim->get_ent(defender.get_parent()).set_saving_progress(0);
    }
    if (defender.has_component(kFlower) && type != DamageType::kPassive)
        defender.set_saving_timer(fclamp(defender.get_saving_timer() - 10 * damage_dealt, 0, PROGRESS_SAVE_DELAY * TPS));
    if (!sim->ent_exists(atk_id)) return;
    Entity &attacker = sim->get_ent(atk_id);

    if (type != DamageType::kReflect && type != DamageType::kSponge && defender.damage_reflection > 0)
        inflict_damage(sim, def_id, attacker.base_entity, damage_dealt * defender.damage_reflection, DamageType::kReflect);

    if (type == DamageType::kContact && defender.get_revived() == 0) {
        if (defender.poison_ticks < attacker.poison_damage.time * TPS) {
            defender.poison_ticks = attacker.poison_damage.time * TPS;
            defender.poison_inflicted = attacker.poison_damage.damage / TPS;
            defender.poison_dealer = attacker.base_entity;
        }

        if (defender.slow_ticks < attacker.slow_inflict)
            defender.slow_ticks = attacker.slow_inflict;

        if (attacker.has_component(kPetal)) {
            if (attacker.get_petal_id() == PetalID::kDandelion && defender.dandy_ticks < 10 * TPS)
                defender.dandy_ticks = 10 * TPS;
            else if (attacker.get_petal_id() == PetalID::kHoney && defender.honey_ticks < 5 * TPS)
                defender.honey_ticks = 5 * TPS;
        }
    }

    if (!sim->ent_alive(defender.target)) {
        if (attacker.has_component(kPetal) && sim->ent_alive(attacker.get_parent()))
            defender.target = attacker.get_parent();
        else if (sim->ent_alive(atk_id))
            defender.target = atk_id;
    }
    defender.last_damaged_by = attacker.base_entity;
}

void inflict_lightning(Simulation *sim, Entity &attacker, Entity &first, float amt, uint32_t bounces) {
    DEBUG_ONLY(assert(bounces <= MAX_LIGHTNING_BOUNCES);)
    StaticArray<Entity *, MAX_LIGHTNING_BOUNCES + 1> chain = {&attacker, &first};
    inflict_damage(sim, attacker.id, first.id, amt, DamageType::kLightning);
    for (uint32_t i = 0; i < bounces - 1; ++i) {
        Entity &last = *chain[chain.size() - 1];
        EntityID target = find_nearest_enemy_to_strike(sim, attacker, last, LIGHTNING_STRIKE_RADIUS, [&](Entity const &ent){
            for (Entity *entity : chain)
                if (ent.id == entity->id) return false;
            return true;
        });
        if (target == NULL_ENTITY) break;
        chain.push(&sim->get_ent(target));
        inflict_damage(sim, attacker.id, target, amt, DamageType::kLightning);
    }
    Entity *last = nullptr;
    for (Entity *ent : chain) {
        Entity &curr = sim->alloc_ent();
        curr.add_component(kPhysics);
        curr.set_x(ent->get_x());
        curr.set_y(ent->get_y());
        curr.add_component(kSegmented);
        if (last != nullptr) {
            curr.set_seg_head(last->id);
            last->set_seg_tail(curr.id);
        }
        curr.add_component(kAnimation);
        curr.set_anim_type(AnimationType::kLightning);
        sim->spatial_hash.insert(curr);
        sim->request_delete(curr.id);
        last = &curr;
    }
}

void inflict_heal(Simulation *sim, Entity &ent, float amt) {
    DEBUG_ONLY(assert(ent.has_component(kHealth));)
    if (ent.pending_delete || ent.health <= 0) return;
    if (ent.dandy_ticks > 0) return;
    if (BitMath::at(ent.flags, EntityFlags::kZombie)) return;
    ent.health = fclamp(ent.health + amt, 0, ent.max_health);
}