#pragma once

class Simulation;
class Entity;

void tick_ai_behavior(Simulation *, Entity &);
void tick_camera_behavior(Simulation *, Entity &);
void tick_chat_behavior(Simulation *, Entity &);
void tick_curse_behavior(Simulation *);
void tick_luck_behavior(Simulation *);
void tick_culling_behavior(Simulation *, Entity &);
void tick_drop_behavior(Simulation *, Entity &);
void tick_entity_motion(Simulation *, Entity &);
void tick_health_behavior(Simulation *, Entity &);
void tick_petal_behavior(Simulation *, Entity &);
void tick_player_behavior(Simulation *, Entity &);
void tick_player_ai_behavior(Simulation *, Entity &);
void tick_stab_behavior(Simulation *, Entity &);
// devStabOSC (the first argument) hit the second
void apply_stab(Simulation *, Entity &, Entity &);
// the shells of the Tank mob explode when they hit something or reach a wall
void tank_shell_hit(Simulation *, Entity &);
void tick_tank_shell(Simulation *, Entity &);
// the special ability of a boss mob (Process/Boss.cc), after its normal behavior
void tick_boss_ability(Simulation *, Entity &);
void tick_tracking_behavior(Simulation *);
void tick_segment_behavior(Simulation *, Entity &);
void tick_score_behavior(Simulation *, Entity &);
void on_collide(Simulation *, Entity &, Entity &);
