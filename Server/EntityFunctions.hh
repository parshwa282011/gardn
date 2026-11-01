#pragma once

#include <Shared/EntityDef.hh>

#include <functional>

class Simulation;
class Entity;
class EntityID;

void inflict_damage(Simulation *, EntityID const, EntityID const, float, uint8_t);
void inflict_lightning(Simulation *, Entity &, Entity &, float, uint32_t);
void inflict_heal(Simulation *, Entity &, float);
void inflict_sheild(Simulation *, Entity &, float);

void entity_on_death(Simulation *, Entity const &);

EntityID find_nearest_enemy(Simulation *, Entity const &, float);
EntityID find_nearest_enemy_within_angle(Simulation *, Entity const &, float, float);
EntityID find_nearest_enemy_to_strike(Simulation *, Entity const &, Entity const &, float, std::function<bool(Entity const &)>);
std::vector<EntityID> find_enemies_to_radiate(Simulation *, Entity const &, float);
EntityID find_teammate_to_heal(Simulation *, Entity const &, float);
EntityID find_teammate_to_shield(Simulation *, Entity const &, float, float);
EntityID find_nearest_magnet(Simulation *, Entity const &, float);

void entity_set_despawn_tick(Entity &, game_tick_t);
void entity_clear_references(Simulation *, Entity &);

void delete_petal(Simulation *, Entity &, PetalID::T);
void pickup_drop(Simulation *, Entity &, Entity &);
game_tick_t get_sponge_period(Simulation *, Entity &);
