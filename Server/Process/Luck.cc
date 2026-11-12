#include <Server/Process.hh>

#include <Shared/Entity.hh>
#include <Shared/Map.hh>
#include <Shared/Simulation.hh>

#include <algorithm>

// The luck of every zone, worked out again each tick from the clovers of the players in
// it: nothing is kept from the tick before, so it is constant while the same clovers are
// there. A clover counts while it is in a slot of the loadout, broken or not.
void tick_luck_behavior(Simulation *sim) {
    std::array<float, MAP_DATA.size()> mob_luck = {};
    std::array<float, MAP_DATA.size()> drop_luck = {};
    sim->for_each<kFlower>([&](Simulation *, Entity &player) {
        if (player.pending_delete || player.has_component(kMob)) return;
        // (the petals do nothing while a flower is too strong for its zone)
        if (player.get_overlevel_timer() >= PETAL_DISABLE_DELAY * TPS) return;
        uint32_t zone = Map::get_zone_from_pos(player.get_x(), player.get_y());
        for (uint32_t i = 0; i < player.get_loadout_count(); ++i) {
            struct PetalAttributes const &attrs = PETAL_DATA[player.loadout[i].get_petal_id()].attributes;
            mob_luck[zone] += attrs.mob_luck;
            drop_luck[zone] += attrs.drop_luck;
        }
    });
    for (uint32_t zone = 0; zone < MAP_DATA.size(); ++zone) {
        sim->zone_mob_luck[zone] = std::min(mob_luck[zone], MAX_ZONE_LUCK);
        sim->zone_drop_luck[zone] = std::min(drop_luck[zone], MAX_ZONE_LUCK);
    }
}
