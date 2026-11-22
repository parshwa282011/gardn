#pragma once

#include <Shared/Entity.hh>

#include <functional>

class Simulation;

Entity &alloc_drop(Simulation *, PetalID::T);
// boss = true: a boss (BOSS_* in StaticDefinitions.hh). The ants that come with a hole are never bosses
Entity &alloc_mob(
    Simulation *, MobID::T, float, float, 
    EntityID const, std::function<void(Entity &)> = nullptr, bool boss = false
);
// every kind of mob can be a boss; the ones that come by themselves now and then are the creatures
// (not the rocks, the holes, the digger...)
bool mob_can_be_boss(MobID::T);
bool mob_is_natural_boss(MobID::T);
uint32_t count_bosses(Simulation *);
Entity &alloc_player(Simulation *, EntityID const);
// the /summon chat command: a mob a bit far from the player, in a random direction
Entity &summon_mob_near(Simulation *, Entity const &, MobID::T, bool boss = false);
Entity &alloc_petal(Simulation *, PetalID::T, Entity const &);
Entity &alloc_web(Simulation *, float, Entity const &);
Entity &alloc_chat(Simulation *, std::string &, Entity const &);
Entity &alloc_dot(Simulation *, Entity const &);
// the dot an observer shows for a mob: it belongs to the camera and follows the mob
Entity &alloc_tracker_dot(Simulation *, Entity const &camera, Entity const &mob);
Entity &alloc_progress(Simulation *, struct PlayerProgress &, Entity const &);
Entity &alloc_camera(Simulation *, EntityID const);
Entity &alloc_cpu_camera(Simulation *, EntityID const);

void player_spawn(Simulation *, Entity &, Entity &);