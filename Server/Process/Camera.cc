#include <Server/Process.hh>

#include <Server/Client.hh>
#include <Server/EntityFunctions.hh>
#include <Server/PetalTracker.hh>
#include <Server/Server.hh>

#include <Shared/Entity.hh>
#include <Shared/Map.hh>
#include <Shared/Simulation.hh>
#include <Shared/StaticData.hh>

void tick_camera_behavior(Simulation *sim, Entity &ent) {
    if (sim->ent_exists(ent.get_player())) {
        Entity &player = sim->get_ent(ent.get_player());
        ent.set_camera_x(player.get_x());
        ent.set_camera_y(player.get_y());
        player.set_loadout_count(loadout_slots_at_level(score_to_level(player.get_score())));
        bool saving_penalty = false;
        if (player.acceleration.x != 0 || player.acceleration.y != 0) {
            player.set_angle(player.acceleration.angle());
            saving_penalty = true;
        }

        ent.last_damaged_by = player.last_damaged_by;
        struct ZoneDefinition const &zone = MAP_DATA[Map::get_zone_from_pos(player.get_x(), player.get_y())];
        if (zone.difficulty < Map::difficulty_at_level(score_to_level(player.get_score()))) {
            if (player.get_overlevel_timer() < PETAL_DISABLE_DELAY * TPS)
                player.set_overlevel_timer(player.get_overlevel_timer() + 1);
            else player.set_overlevel_timer(PETAL_DISABLE_DELAY * TPS);
            saving_penalty = true;
            ent.set_saving_progress(0);
        } else {
            if (player.get_overlevel_timer() > 0)
                player.set_overlevel_timer(player.get_overlevel_timer() - 0.1);
            else player.set_overlevel_timer(0);
        }
        if (BitMath::at(ent.flags, EntityFlags::kZombie)) {
            BitMath::set(player.flags, EntityFlags::kZombie);
            player.input = 0;
            player.acceleration.set(0, 0);
            float dmg = player.max_health / (60 * TPS);
            inflict_damage(sim, NULL_ENTITY, player.id, dmg, DamageType::kPassive);
            saving_penalty = true;
            ent.set_saving_progress(0);
        } else
            BitMath::unset(player.flags, EntityFlags::kZombie);
        bool has_legendary = false;
        for (uint8_t i = 0; i < player.get_loadout_count() + MAX_SLOT_COUNT; ++i) {
            if (PETAL_DATA[player.get_loadout_ids(i)].rarity >= RarityID::kLegendary) {
                has_legendary = true;
                break;
            }
        }
        if (ent.client == nullptr || ent.client->account_uuid.empty() ||
            player.get_saving_timer() == 0 || ent.progress_saves.size() >= MAX_PROGRESS_SAVES ||
            (score_to_level(player.get_score()) < 45 && !has_legendary))
            ent.set_saving_progress(0);
        player.set_saving_timer(fclamp(player.get_saving_timer() + (saving_penalty ? -10 : 1), 0, PROGRESS_SAVE_DELAY * TPS));
        if (ent.get_saving_progress() && player.get_saving_timer() == PROGRESS_SAVE_DELAY * TPS && sim->ent_alive(player.id))
            ent.client->game->save_progress(ent.client);
    } else {
        if (BitMath::at(ent.flags, EntityFlags::kCPUControlled)) {
            //temp: cpu cameras die
            return sim->request_delete(ent.id);
        }
        if (BitMath::at(ent.flags, EntityFlags::kZombie) &&
            !BitMath::at(ent.flags, EntityFlags::kIsDespawning))
            entity_set_despawn_tick(ent, 90 * TPS);
        ent.set_player(NULL_ENTITY);
        if (!ent.get_saving_progress())
            ent.set_fov(BASE_FOV * 0.9);
        if (sim->ent_exists(ent.last_damaged_by)){
            Entity &spectating = sim->get_ent(ent.last_damaged_by);
            ent.set_camera_x(spectating.get_x());
            ent.set_camera_y(spectating.get_y());
        }
    }
}
