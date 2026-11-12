#include <Server/Process.hh>

#include <Server/EntityFunctions.hh>
#include <Server/Spawn.hh>

#include <Shared/Entity.hh>
#include <Shared/Simulation.hh>

#include <set>
#include <vector>

// The observer: the mobs the player selected in the mob gallery show up as dots on
// their minimap. Every dot belongs to the camera of the player and follows its mob.

static bool _is_tracked(Entity const &player, MobID::T mob_id) {
    return mob_id < MobID::kNumMobs && BitMath::at(player.tracked_mob_mask[mob_id / 8], mob_id % 8);
}

static void _tick_camera_tracking(Simulation *sim, Entity &camera) {
    bool active = false;
    if (sim->ent_alive(camera.get_player())) {
        Entity const &player = sim->get_ent(camera.get_player());
        active = BitMath::at(player.get_equip_flags(), EquipmentFlags::kObserver);
    }
    Entity const *player = active ? &sim->get_ent(camera.get_player()) : nullptr;

    // the dots that are still good move with their mob, the others go
    std::set<EntityID> followed;
    std::vector<EntityID> &dots = camera.tracker_dots;
    for (size_t i = dots.size(); i > 0; --i) {
        EntityID dot_id = dots[i - 1];
        bool keep = false;
        if (sim->ent_alive(dot_id)) {
            Entity &dot = sim->get_ent(dot_id);
            if (active && sim->ent_alive(dot.tracked_target)) {
                Entity const &mob = sim->get_ent(dot.tracked_target);
                if (_is_tracked(*player, mob.get_mob_id())) {
                    dot.set_x(mob.get_x());
                    dot.set_y(mob.get_y());
                    followed.insert(mob.id);
                    keep = true;
                }
            }
            if (!keep) sim->request_delete(dot_id);
        }
        if (!keep) {
            dots[i - 1] = dots.back();
            dots.pop_back();
        }
    }
    if (!active || dots.size() >= MAX_TRACKED_MOBS) return;

    // the mobs that have no dot yet (a body segment follows its head: only the head counts)
    std::vector<EntityID> missing;
    sim->for_each<kMob>([&](Simulation *sim, Entity &mob) {
        if (dots.size() + missing.size() >= MAX_TRACKED_MOBS) return;
        if (mob.pending_delete || !_is_tracked(*player, mob.get_mob_id())) return;
        if (mob.has_component(kSegmented) && sim->ent_alive(mob.get_seg_head())) return;
        if (followed.contains(mob.id)) return;
        missing.push_back(mob.id);
    });
    for (EntityID mob_id : missing)
        dots.push_back(alloc_tracker_dot(sim, camera, sim->get_ent(mob_id)).id);
}

void tick_tracking_behavior(Simulation *sim) {
    // a dot whose camera is gone (the player left) is gone too
    sim->for_each<kDot>([](Simulation *sim, Entity &dot) {
        if (dot.get_tracked_mob() == 0 || dot.pending_delete) return;
        if (!sim->ent_alive(dot.get_parent())) sim->request_delete(dot.id);
    });
    sim->for_each<kCamera>(_tick_camera_tracking);
}
