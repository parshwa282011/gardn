#include <Shared/Map.hh>

#ifdef SERVERSIDE
#include <Server/Spawn.hh>
#include <Shared/Entity.hh>
#endif

#include <cmath>
#include <iostream>

using namespace Map;

uint32_t Map::difficulty_at_level(uint32_t level) {
    if (level / LEVELS_PER_EXTRA_SLOT > MAX_DIFFICULTY) return MAX_DIFFICULTY;
    return level / LEVELS_PER_EXTRA_SLOT;
}

uint32_t Map::get_zone_from_pos(float x, float y) {
    uint32_t ret = 0;
    for (uint32_t i = 1; i < MAP_DATA.size(); ++i) {
        struct ZoneDefinition const &zone = MAP_DATA[i];
        if (fclamp(x, zone.left, zone.right) == x && fclamp(y, zone.top, zone.bottom) == y)
            ret = i;
    }
    return ret;
}

uint32_t Map::get_suitable_difficulty_zone(uint32_t power) {
    StaticArray<uint32_t, MAP_DATA.size()> possible_zones;
    for (uint32_t i = 0; i < MAP_DATA.size(); ++i)
        if (MAP_DATA[i].difficulty == power) possible_zones.push(i);
    DEBUG_ONLY(assert(possible_zones.size() > 0);)
    return possible_zones[frand() * possible_zones.size()];
}

uint32_t Map::get_random_zone_with_spawns() {
    StaticArray<uint32_t, MAP_DATA.size()> possible_zones;
    for (uint32_t i = 0; i < MAP_DATA.size(); ++i)
        if (MAP_DATA[i].spawns.size() > 0) possible_zones.push(i);
    DEBUG_ONLY(assert(possible_zones.size() > 0);)
    return possible_zones[frand() * possible_zones.size()];
}

#ifdef SERVERSIDE
#include <Shared/Simulation.hh>
void Map::remove_mob(Simulation *sim, uint32_t zone) {
    DEBUG_ONLY(assert(zone < MAP_DATA.size());)
    --sim->zone_mob_counts[zone];
}

// Every mob can spawn in the PvP area of the sandbox. A mob that the map spawns keeps the
// chance it has there (added up over the zones); the ones the map never spawns by
// themselves (the worker, soldier and queen ants, the digger) are as rare as the rarest.
static StaticArray<SpawnChance, MobID::kNumMobs> const &_sandbox_spawns() {
    static StaticArray<SpawnChance, MobID::kNumMobs> const spawns = [](){
        float weight[MobID::kNumMobs] = {0};
        for (struct ZoneDefinition const &zone : MAP_DATA)
            for (SpawnChance const &s : zone.spawns)
                weight[s.id] += s.chance;
        float rarest = 0;
        for (MobID::T id = 0; id < MobID::kNumMobs; ++id)
            if (weight[id] > 0 && (rarest == 0 || weight[id] < rarest)) rarest = weight[id];
        StaticArray<SpawnChance, MobID::kNumMobs> ret;
        for (MobID::T id = 0; id < MobID::kNumMobs; ++id)
            ret.push({ id, weight[id] > 0 ? weight[id] : rarest });
        return ret;
    }();
    return spawns;
}

float Map::drop_luck_at(Simulation *sim, float x, float y) {
    return sim->zone_drop_luck[get_zone_from_pos(x, y)] / MAX_ZONE_LUCK;
}

float Map::drop_multiplier(float luck) {
    return 1 + LUCK_DROP_BONUS * luck;
}

void Map::spawn_random_mob(Simulation *sim, float x, float y) {
    uint32_t zone_id = Map::get_zone_from_pos(x, y);
    struct ZoneDefinition const &zone = MAP_DATA[zone_id];
    float density = zone.density;
    float area = (zone.right - zone.left) * (zone.bottom - zone.top);
    if (sim->arena_info.gamemode == Gamemode::kSandbox) {
        // the PvP area of the sandbox (the only place that spawns mobs there)
        density = SANDBOX_PVP_DENSITY;
        area = (SANDBOX_WIDTH - SANDBOX_PVP_LEFT) * SANDBOX_HEIGHT;
    }
    if (density * area / (500 * 500) < sim->zone_mob_counts[zone_id]) return;
    auto spawns = sim->arena_info.gamemode == Gamemode::kSandbox ? _sandbox_spawns() :
        zone.spawns.size() > 0 ? zone.spawns : MAP_DATA[Map::get_random_zone_with_spawns()].spawns;
    float sum = 0;
    for (SpawnChance const &s : spawns)
        sum += s.chance;
    sum *= frand();
    for (SpawnChance const &s : spawns) {
        sum -= s.chance;
        if (sum <= 0) {
            Entity &ent = alloc_mob(sim, s.id, x, y, NULL_ENTITY, [&](Entity &mob){
                mob.zone = zone_id;
                mob.immunity_ticks = TPS;
                BitMath::set(mob.flags, EntityFlags::kSpawnedFromZone);
                BitMath::set(mob.flags, EntityFlags::kHasCulling);
                sim->zone_mob_counts[zone_id]++;
                mob.score_reward = MOB_DATA[mob.get_mob_id()].xp;
            });
            return;
        }
    }
}

bool Map::find_spawn_location(Simulation *sim, float d, Vector &vref) {
    for (uint32_t i = 0; i < 10; ++i) {
        vref.set(frand() * ARENA_WIDTH, frand() * ARENA_HEIGHT);
        bool valid = true;
        sim->for_each<kFlower>([&](Simulation *, Entity &ent) {
            if (ent.has_component(kMob)) return;
            if (!valid) return;
            if (Vector(ent.get_x() - vref.x, ent.get_y() - vref.y).magnitude() < d) 
                valid = false;
        });
        if (valid) return true;
    }
    return false;
}
#endif