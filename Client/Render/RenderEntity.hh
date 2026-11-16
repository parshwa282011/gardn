#pragma once

class Entity;
class Renderer;
class Simulation;

void render_animation(Simulation *, Renderer &, Entity const &);
void render_chat(Renderer &, Entity const &);
void render_drop(Renderer &, Entity const &);
void render_flower(Renderer &, Entity const &);
void render_health(Renderer &, Entity const &);
void render_mob(Renderer &, Entity const &);
void render_name(Renderer &, Entity const &);
void render_petal(Renderer &, Entity const &);
// devStabOSC: the circle under the flower, and the effect over it
void render_stab_ground(Renderer &, Entity const &);
void render_stab_effect(Renderer &, Entity const &);
void render_web(Renderer &, Entity const &);
