#include <Server/Process.hh>

#include <Server/EntityFunctions.hh>
#include <Server/Spawn.hh>

#include <Shared/Entity.hh>
#include <Shared/Simulation.hh>
#include <Shared/StaticData.hh>

#include <algorithm>
#include <cmath>

// The special abilities of the bosses (mobs with the boss tag: 10x the health, 2x the damage, 5x
// the size, see make_boss in Spawn.cc). Each kind of mob has its own, built from a few parts:
//   ring    missiles in all directions                 fan      missiles at the target
//   charge  a run in a straight line at the target     summon   minions that stay with the boss
//   webs    webs shot at the target / a ring of webs   predict  running to where the target is going
// Below half health a boss is enraged: its timed abilities come 1.6 times as often.
// This runs after the normal behavior of the mob (Ai.cc), once per tick, for the head of a segmented
// mob. A few abilities live somewhere else: the Fire Ant's 5% damage back (make_boss + inflict_damage),
// the Shiny Ladybug's revival (inflict_damage), what the holes and burrows let out (inflict_damage,
// Ai.cc), the Digger boss' war with everything (Ai.cc, Collision.cc).

static float const BOSS_MISSILE_DAMAGE = 20;
// how hard a charge pushes (times the acceleration of a mob that chases): about 2.4x its speed
static float const CHARGE_ACCELERATION = 2.4f;

static void _missile(Simulation *sim, Entity &boss, float angle, float speed = 40) {
    Entity &missile = alloc_petal(sim, PetalID::kMissile, boss);
    missile.damage = BOSS_MISSILE_DAMAGE;
    missile.health = missile.max_health = 10;
    entity_set_despawn_tick(missile, 3 * TPS);
    missile.set_angle(angle);
    // (at the edge of the boss, not inside it)
    float out = boss.get_radius() + missile.get_radius() + 4;
    missile.set_x(boss.get_x() + cosf(angle) * out);
    missile.set_y(boss.get_y() + sinf(angle) * out);
    missile.acceleration.unit_normal(angle).set_magnitude(speed * PLAYER_ACCELERATION);
}

static void _ring(Simulation *sim, Entity &boss, uint32_t count) {
    float start = frand() * 2 * M_PI;
    for (uint32_t i = 0; i < count; ++i)
        _missile(sim, boss, start + i * 2 * M_PI / count);
}

static void _fan(Simulation *sim, Entity &boss, float aim, uint32_t count, float spread) {
    for (uint32_t i = 0; i < count; ++i) {
        float t = count == 1 ? 0.5f : (float) i / (count - 1);
        _missile(sim, boss, aim - spread / 2 + spread * t);
    }
}

// A charge: for `seconds` the boss runs in a straight line, the way the target was when it began
static void _start_charge(Entity &boss, float aim, float seconds) {
    boss.boss_charge_ticks = (game_tick_t) (seconds * TPS);
    boss.boss_charge_angle = aim;
}

static bool _charging(Entity const &boss) { return boss.boss_charge_ticks > 0; }

static void _tick_charge(Entity &boss) {
    if (boss.boss_charge_ticks == 0) return;
    --boss.boss_charge_ticks;
    boss.acceleration.unit_normal(boss.boss_charge_angle).set_magnitude(PLAYER_ACCELERATION * CHARGE_ACCELERATION);
    boss.set_angle(boss.boss_charge_angle);
}

static uint32_t _count_minions(Simulation *sim, Entity const &boss) {
    uint32_t n = 0;
    sim->for_each<kMob>([&](Simulation *, Entity &mob) {
        if (!mob.pending_delete && mob.get_parent() == boss.id) ++n;
    });
    return n;
}

// The minions are normal mobs of the kind that stay with the boss, go when it goes, pay nothing, and
// never more than BOSS_MAX_MINIONS at the same time. A minion that is a boss itself (`as_boss`: the
// rocks of a boulder, the boulders of a mountain) is a boss like any other and pays like one
static void _summon(Simulation *sim, Entity &boss, MobID::T id, uint32_t count, bool as_boss = false) {
    uint32_t alive = _count_minions(sim, boss);
    for (uint32_t i = 0; i < count && alive < BOSS_MAX_MINIONS; ++i, ++alive) {
        Vector spot = Vector::rand(boss.get_radius());
        Entity &minion = alloc_mob(sim, id, boss.get_x() + spot.x, boss.get_y() + spot.y, boss.get_team(), [as_boss](Entity &mob) {
            BitMath::set(mob.flags, EntityFlags::kHasCulling);
            BitMath::set(mob.flags, EntityFlags::kDieOnParentDeath);
            if (as_boss) mob.score_reward = MOB_DATA[mob.get_mob_id()].xp;
            else BitMath::set(mob.flags, EntityFlags::kNoDrops);
        }, as_boss);
        minion.set_parent(boss.id);
        minion.target = boss.target;
    }
}

// a web shot at the target: it flies and slows down, then stays for a while
static void _web_shot(Simulation *sim, Entity &boss, float aim) {
    Entity &web = alloc_web(sim, 45, boss);
    float out = boss.get_radius() + 50;
    web.set_x(boss.get_x() + cosf(aim) * out);
    web.set_y(boss.get_y() + sinf(aim) * out);
    web.friction = 0.12f;
    web.velocity.unit_normal(aim).set_magnitude(70);
}

// webs all around, where they land they stay
static void _web_ring(Simulation *sim, Entity &boss, uint32_t count) {
    float start = frand() * 2 * M_PI;
    float distance = boss.get_radius() + 170;
    for (uint32_t i = 0; i < count; ++i) {
        float angle = start + i * 2 * M_PI / count;
        Entity &web = alloc_web(sim, 55, boss);
        web.set_x(boss.get_x() + cosf(angle) * distance);
        web.set_y(boss.get_y() + sinf(angle) * distance);
    }
}

// ------------------------------------------------------------------ the beetles
// The beetles run to where the player is going, not where it is. The last BossHistory::kLength
// positions of the target are kept (one per tick: a second), the derivatives of the position
// are taken up to the 10th from them (backward differences) and the Newton series gives the position
// a few ticks from now:   p(n+s) = sum over k of  s(s+1)...(s+k-1)/k!  * (the k-th difference at n).
// (A high order on noisy data is wild, so the guess is kept near the target and a bad number
// falls back to the position itself.)
static void _record_target(Entity &boss, Entity const &target) {
    BossHistory &h = boss.boss_history;
    if (!(h.of == target.id)) {
        h = {};
        h.of = target.id;
    }
    h.x[h.head] = target.get_x();
    h.y[h.head] = target.get_y();
    h.head = (h.head + 1) % BossHistory::kLength;
    if (h.count < BossHistory::kLength) ++h.count;
}

static Vector _predict(Entity const &boss, Entity const &target, float steps) {
    BossHistory const &h = boss.boss_history;
    uint32_t n = h.count;
    Vector now(target.get_x(), target.get_y());
    if (n < 3 || steps <= 0) return now;
    // oldest to newest
    float px[BossHistory::kLength], py[BossHistory::kLength];
    for (uint32_t i = 0; i < n; ++i) {
        uint32_t at = (h.head + BossHistory::kLength - n + i) % BossHistory::kLength;
        px[i] = h.x[at];
        py[i] = h.y[at];
    }
    uint32_t order = std::min<uint32_t>(10, n - 1);
    // the differences, in place: after k rounds d[n-1] is the k-th backward difference at the newest
    float dx[BossHistory::kLength], dy[BossHistory::kLength];
    std::copy(px, px + n, dx);
    std::copy(py, py + n, dy);
    float result_x = dx[n - 1], result_y = dy[n - 1];
    float coefficient = 1;
    for (uint32_t k = 1; k <= order; ++k) {
        for (uint32_t i = n - 1; i >= k; --i) {
            dx[i] = dx[i] - dx[i - 1];
            dy[i] = dy[i] - dy[i - 1];
        }
        coefficient *= (steps + k - 1) / (float) k;
        result_x += coefficient * dx[n - 1];
        result_y += coefficient * dy[n - 1];
    }
    if (!std::isfinite(result_x) || !std::isfinite(result_y)) return now;
    Vector guess(result_x, result_y);
    Vector off(guess.x - now.x, guess.y - now.y);
    // not further than the target could be going in those ticks, at a generous speed
    float limit = std::min(900.0f, steps * 3 * PLAYER_ACCELERATION * 3);
    if (off.magnitude() > limit) off.set_magnitude(limit);
    return Vector(now.x + off.x, now.y + off.y);
}

static void _beetle(Simulation *sim, Entity &boss, Entity *target) {
    if (target == nullptr) {
        boss.boss_history.count = 0;
        return;
    }
    _record_target(boss, *target);
    float dx = target->get_x() - boss.get_x(), dy = target->get_y() - boss.get_y();
    float dist = sqrtf(dx * dx + dy * dy);
    // how many ticks it takes to get there at its speed (about 3 times its acceleration)
    float steps = std::min(20.0f, std::max(1.0f, dist / (PLAYER_ACCELERATION * 3)));
    Vector where = _predict(boss, *target, steps);
    Vector run(where.x - boss.get_x(), where.y - boss.get_y());
    if (run.magnitude() < 1) return;
    boss.acceleration = run;
    boss.acceleration.set_magnitude(PLAYER_ACCELERATION * 1.05f);
    boss.set_angle(run.angle());
}

// ------------------------------------------------------------------ the others
static void _cactus(Entity &boss) {
    // slowly 1x to BOSS_CACTUS_SWELL times its size and slowly back
    float period = BOSS_CACTUS_PERIOD_SECONDS * TPS;
    float phase = 0.5f - 0.5f * cosf(2 * M_PI * (boss.boss_tick % (uint32_t) period) / period);
    boss.set_radius(boss.boss_base_radius * (1 + (BOSS_CACTUS_SWELL - 1) * phase));
}

// the Digger boss: once a second, 1 damage to every mob and flower around it (not to itself)
static void _digger(Simulation *sim, Entity &boss) {
    std::vector<EntityID> hit;
    sim->spatial_hash.query(boss.get_x(), boss.get_y(), URANIUM_RADIATION_RADIUS, URANIUM_RADIATION_RADIUS, [&](Simulation *s, Entity &ent) {
        if (!s->ent_alive(ent.id) || ent.id == boss.id) return;
        if (!ent.has_component(kMob) && !ent.has_component(kFlower)) return;
        if (!ent.has_component(kHealth) || ent.health <= 0) return;
        float dx = ent.get_x() - boss.get_x(), dy = ent.get_y() - boss.get_y();
        if (dx * dx + dy * dy > URANIUM_RADIATION_RADIUS * URANIUM_RADIATION_RADIUS) return;
        hit.push_back(ent.id);
    });
    for (EntityID id : hit)
        inflict_damage(sim, boss.id, id, BOSS_DIGGER_RADIATION_DAMAGE, DamageType::kUranium);
    Entity &anim = sim->alloc_ent();
    anim.add_component(kPhysics);
    anim.set_x(boss.get_x());
    anim.set_y(boss.get_y());
    anim.set_radius(URANIUM_RADIATION_RADIUS);
    anim.add_component(kAnimation);
    anim.set_anim_type(AnimationType::kUranium);
    sim->spatial_hash.insert(anim);
    sim->request_delete(anim.id);
}

void tick_boss_ability(Simulation *sim, Entity &boss) {
    ++boss.boss_tick;
    _tick_charge(boss);
    // the Yggdrasil of the Shiny Ladybug boss reloads (the client shows how far it is)
    if (boss.boss_reload_ticks > 0) {
        --boss.boss_reload_ticks;
        uint32_t total = BOSS_YGG_RELOAD_SECONDS * TPS;
        boss.set_boss_reload((uint8_t) (255 * (total - boss.boss_reload_ticks) / total));
    }
    bool const enraged = boss.health < boss.max_health * 0.5f;
    float const pace = enraged ? 0.625f : 1.0f;
    // true once every `seconds` (shorter when enraged); `offset` keeps two abilities off the same tick
    auto every = [&](float seconds, uint32_t offset) {
        uint32_t period = std::max<uint32_t>(2, (uint32_t) (seconds * pace * TPS));
        return boss.boss_tick % period == offset % period;
    };
    Entity *target = sim->ent_alive(boss.target) ? &sim->get_ent(boss.target) : nullptr;
    float aim = boss.get_angle();
    float dist = 0;
    if (target != nullptr) {
        float dx = target->get_x() - boss.get_x(), dy = target->get_y() - boss.get_y();
        aim = atan2f(dy, dx);
        dist = sqrtf(dx * dx + dy * dy);
    }
    switch (boss.get_mob_id()) {
        // ants: they run in a straight line at whoever they are after for 3 s, and bring three more of themselves
        case MobID::kBabyAnt:
        case MobID::kWorkerAnt:
        case MobID::kSoldierAnt:
            if (target && !_charging(boss) && every(6, 0)) _start_charge(boss, aim, 3);
            if (every(15, 100)) _summon(sim, boss, boss.get_mob_id(), 3);
            break;
        case MobID::kQueenAnt:
            // she makes soldier ants like she always does (Ai.cc), and now another queen as well
            if (every(10, 100)) _summon(sim, boss, MobID::kQueenAnt, 1);
            break;
        case MobID::kFireAnt:
            // (it gives 5% of what it takes back: make_boss)
            break;
        case MobID::kBee:
            if (target && !_charging(boss) && every(4, 0)) _start_charge(boss, aim, 1.5f);
            break;
        case MobID::kHornet:
            // a fan of missiles aimed at the target, and a ring
            if (target && dist < 1600 && every(1.8, 0)) _fan(sim, boss, aim, 5, 0.9f);
            if (every(7, 60)) _ring(sim, boss, enraged ? 18 : 12);
            break;
        case MobID::kTank:
            break;
        case MobID::kLadybug:
        case MobID::kMassiveLadybug:
        case MobID::kDarkLadybug:
            if (every(10, 100)) _summon(sim, boss, MobID::kSpider, 2);
            break;
        case MobID::kShinyLadybug:
            // (its Yggdrasil is in inflict_damage, the reload above)
            break;
        case MobID::kBeetle:
        case MobID::kMassiveBeetle:
            _beetle(sim, boss, target);
            break;
        case MobID::kScorpion:
            if (target && dist < 1600 && every(1.6, 0)) _fan(sim, boss, aim, 3, 0.45f);
            if (target && !_charging(boss) && every(6, 40)) _start_charge(boss, aim, 1.5f);
            break;
        case MobID::kSpider:
            if (target && dist < 1600 && every(1.5, 0)) _web_shot(sim, boss, aim);
            if (every(6, 50)) _web_ring(sim, boss, enraged ? 14 : 10);
            if (target && !_charging(boss) && every(7, 90)) _start_charge(boss, aim, 1.5f);
            break;
        case MobID::kCentipede:
        case MobID::kEvilCentipede:
        case MobID::kDesertCentipede:
            if (target && !_charging(boss) && every(5, 0)) _start_charge(boss, aim, 2);
            break;
        case MobID::kSandstorm:
            if (every(10, 100)) _summon(sim, boss, MobID::kSandstorm, 1);
            break;
        case MobID::kBoulder:
            if (every(10, 100)) _summon(sim, boss, MobID::kRock, 1, true);
            break;
        case MobID::kMountain:
            if (every(10, 100)) _summon(sim, boss, MobID::kBoulder, 1, true);
            break;
        case MobID::kCactus:
            _cactus(boss);
            break;
        case MobID::kDigger:
            // (it goes for everything: Ai.cc, Collision.cc)
            if (boss.boss_tick % TPS == 0) _digger(sim, boss);
            break;
        // no ability: Rock, Square, the holes and burrows (they let boss versions out: inflict_damage / Ai.cc)
        default:
            break;
    }
}
