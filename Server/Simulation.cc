#include <Shared/Simulation.hh>

#include <Server/Process.hh>
#include <Server/Client.hh>
#include <Server/EntityFunctions.hh>
#include <Server/Server.hh>
#include <Server/Spawn.hh>
#include <Server/SpatialHash.hh>

#include <Shared/Map.hh>

#include <algorithm>
#include <vector>

static void calculate_leaderboard(Simulation *sim) {
    std::vector<Entity *> players;
    sim->for_each<kCamera>([&](Simulation *sim, Entity &ent) { 
        if (sim->ent_alive(ent.get_player())) {
            Entity &player = sim->get_ent(ent.get_player());
            player.set_leaderboard_pos(LEADERBOARD_SIZE);
            players.push_back(&player);
        }
    });
    std::stable_sort(players.begin(), players.end(), [](Entity const *a, Entity const *b){
        return a->get_score() > b->get_score();
    });
    uint32_t num = players.size();
    sim->arena_info.set_player_count(num);
    num = std::min(num, LEADERBOARD_SIZE);
    for (uint32_t i = 0; i < num; ++i) {
        players[i]->set_leaderboard_pos(i);
        sim->arena_info.set_names(i, players[i]->get_name());
        sim->arena_info.set_scores(i, players[i]->get_score());
        if (sim->arena_info.gamemode == Gamemode::kTDM)
            sim->arena_info.set_colors(i, players[i]->get_color());
        else
            sim->arena_info.set_colors(i, ColorID::kGreen);
    }
}

void Simulation::on_tick() {
    spatial_hash.refresh(ARENA_WIDTH, ARENA_HEIGHT);
    if (arena_info.gamemode == Gamemode::kSandbox) {
        // mobs spawn by themselves only in the PvP area (the right half)
        if (frand() < 1.0f / TPS)
            for (uint32_t i = 0; i < 10 && zone_mob_counts[0] < SANDBOX_MOB_CAP; ++i)
                Map::spawn_random_mob(this, SANDBOX_PVP_LEFT + 200 + frand() * (SANDBOX_WIDTH - SANDBOX_PVP_LEFT - 400), 200 + frand() * (SANDBOX_HEIGHT - 400));
    } else if (frand() < 1.0f / TPS) {
        for (uint32_t i = 0; i < 10; ++i) {
            Vector v;
            if (Map::find_spawn_location(this, 500, v))
                Map::spawn_random_mob(this, v.x, v.y);
        }
    }
    for_each_entity([](Simulation *sim, Entity &ent) {
        DEBUG_ONLY(assert(!(ent.has_component(kAnimation) && sim->ent_alive(ent.id)));)
        if (ent.has_component(kPhysics) && !ent.has_component(kDot))
            sim->spatial_hash.insert(ent);
        if (BitMath::at(ent.flags, EntityFlags::kHasCulling))
            BitMath::set(ent.flags, EntityFlags::kIsCulled);
    });
    for_each<kCamera>(tick_culling_behavior);
    for_each<kFlower>(tick_player_behavior);
    for_each<kMob>(tick_ai_behavior);
    for_each<kCamera>(tick_player_ai_behavior);
    for_each<kPetal>(tick_petal_behavior);
    for_each<kHealth>(tick_health_behavior);
    spatial_hash.collide(on_collide);
    tick_curse_behavior(this);
    for_each<kDrop>(tick_drop_behavior);
    for_each<kPhysics>(tick_entity_motion);
    for_each<kSegmented>(tick_segment_behavior);
    for_each<kCamera>(tick_camera_behavior);
    for_each<kScore>(tick_score_behavior);
    for_each<kChat>(tick_chat_behavior);
    for_each_entity(entity_clear_references);
    calculate_leaderboard(this);
}

void Simulation::post_tick() {
    arena_info.reset_protocol();
    for_each_entity([](Simulation *sim, Entity &ent) {
        //no deletions mid tick
        ent.reset_protocol();
        ++ent.lifetime;
        if (ent.has_component(kHealth)) {
            ent.set_damaged(0);
            ent.set_revived(0);
        }
        ent.chat_sent = NULL_ENTITY;
        if (BitMath::at(ent.flags, EntityFlags::kIsDespawning)) {
            if (ent.despawn_tick == 0) sim->request_delete(ent.id);
            else --ent.despawn_tick;
        }
        if (ent.immunity_ticks > 0) --ent.immunity_ticks;
        if (ent.ghost_ticks > 0) --ent.ghost_ticks;
        if (ent.curse_free_ticks > 0) --ent.curse_free_ticks;
    });
    for_each_entity([](Simulation *sim, Entity &ent) {
        if (!ent.pending_delete) return;
        if (ent.deletion_tick == 0)
            entity_on_death(sim, ent);
        if (!ent.has_component(kPhysics)) 
            return sim->_delete_ent(ent.id);
        if (!ent.has_component(kAnimation) && ent.deletion_tick >= TPS / 5)
            return sim->_delete_ent(ent.id);
        if (ent.deletion_tick >= TPS)
            return sim->_delete_ent(ent.id);
        ++ent.deletion_tick;
    });
}
