#pragma once

#include <Helpers/Array.hh>
#include <Helpers/Math.hh>

#include <cstdint>

inline uint32_t const MAX_LEVEL = 100;
inline uint32_t const TPS = 20;
// how long a new flower in FFA and TDM goes through everything and is ignored by mobs
inline uint32_t const SPAWN_PROTECTION_SECONDS = 10;
inline uint32_t const SPAWN_CURSE_FREE_SECONDS = 10;
inline float const SPAWN_BOTTOM_MARGIN = 100;
inline float const SPAWN_BOTTOM_STRIP = 150;
inline uint32_t const ARENA_WIDTH = 40000;
inline uint32_t const ARENA_HEIGHT = 4000;

inline uint32_t const MAX_SLOT_COUNT = 12;
inline uint32_t const LEVELS_PER_EXTRA_SLOT = 15;
inline uint32_t const LEADERBOARD_SIZE = 10;
inline uint32_t const MAX_PETALS_IN_CLUMP = 5;
inline uint32_t const MAX_DIFFICULTY = 3;
inline uint32_t const MAX_DROPS_PER_MOB = 6;
inline uint32_t const CHAT_SIZE = 5;
inline uint32_t const FIRE_ANT_COUNT = 20;
inline uint32_t const MAX_LIGHTNING_BOUNCES = 15;
// the most mobs an observer shows on the minimap at once
inline uint32_t const MAX_TRACKED_MOBS = 100;
inline uint32_t const MAX_SPONGE_PERIOD = 20 * TPS;

inline uint32_t const MAX_NAME_LENGTH = 16;
inline uint32_t const MAX_CHAT_LENGTH = 64;
// what the chat box and the server accept: a /summon or /restore command is longer than a chat line
inline uint32_t const MAX_COMMAND_LENGTH = 256;
inline uint32_t const MAX_DEV_PWD_LENGTH = 28;
inline uint32_t const MAX_UUID_LENGTH = 36;
inline uint32_t const MAX_PLP_LENGTH = 200;
// constrained by StorageProtocol::MAX_LENGTH and sizeof(OUTGOING_PACKET)
inline uint32_t const MAX_PROGRESS_SAVES = 100;

namespace DamageType {
    enum : uint8_t {
        kPassive,
        kContact,
        kPoison,
        kReflect,
        kLightning,
        kUranium,
        kSponge
    };
}

namespace Gamemode {
    enum : uint8_t {
        kFFA,
        kTDM,
        // a test world: one square, no mobs, nothing is saved to the account
        kSandbox,
        kNumGamemodes
    };
}

// The sandbox is two squares side by side in the corner of the map (inside the first
// zone): the normal area on the left, where no mobs spawn, and the PvP area on the
// right, where they do. The players are kept inside, the client draws only these.
inline uint32_t const SANDBOX_WIDTH = 8000;
inline uint32_t const SANDBOX_HEIGHT = 4000;
inline uint32_t const SANDBOX_PVP_LEFT = SANDBOX_WIDTH / 2;
// The PvP area has 25 times the mob density of the first zone (which is 1): the zones
// keep density * area / (500 * 500) mobs, so 1600 here
inline uint32_t const SANDBOX_PVP_DENSITY = 10;
inline uint32_t const SANDBOX_MOB_CAP = SANDBOX_PVP_DENSITY * (SANDBOX_WIDTH - SANDBOX_PVP_LEFT) * SANDBOX_HEIGHT / (500 * 500);
// Squads (sandbox only, made from the developer menu): at most this many at the same
// time, each with its own flower color. Everybody else is in the normal group (FFA).
inline uint32_t const MAX_SQUADS = 2;
inline uint32_t const SQUAD_CODE_LENGTH = 6;
// what the server tells the player who asked for something about a squad
namespace SquadResult {
    enum : uint8_t {
        kNone,
        kCreated,
        kJoined,
        kLeft,
        kNoSuchSquad,
        kSquadsFull
    };
}

// Proximity voice chat. Players who turned their mic on hear each other by WebRTC (the server
// only passes the handshake along), louder the closer they are: full volume inside
// VOICE_NEAR, silent at VOICE_FAR. A connection is made inside VOICE_CONNECT and dropped
// outside VOICE_DROP (a bit more, so it does not flap at the edge).
inline float const VOICE_NEAR = 250;
inline float const VOICE_FAR = 1200;
inline float const VOICE_CONNECT = 1500;
inline float const VOICE_DROP = 1900;
// handshake messages are small pieces (the client cuts the SDP up) and the server only
// relays them between flowers this close to each other
inline uint32_t const MAX_VOICE_SIGNAL_LENGTH = 500;
inline float const VOICE_SIGNAL_RANGE = 2500;
namespace VoiceState {
    enum : uint8_t {
        kOff,
        kMicOpen,
        kTalking
    };
}

// Bosses: a mob with the boss tag has BOSS_HEALTH_MULTIPLIER times the health, does
// BOSS_DAMAGE_MULTIPLIER times the damage and is BOSS_SIZE_MULTIPLIER times as big, and uses the
// special ability of its kind (Server/Process/Boss.cc). They come now and then by themselves
// (not more than MAX_BOSSES at the same time in a world) and from the "Boss" switch of the developer menu.
inline float const BOSS_HEALTH_MULTIPLIER = 10;
// the Shiny Ladybug boss has the ability of the Yggdrasil: it is revived once, then the ability
// reloads (shown next to its health bar). It has more health, and every revival gives it half of
// the health of the one before
inline float const BOSS_SHINY_HEALTH_MULTIPLIER = 50;
inline uint32_t const BOSS_YGG_RELOAD_SECONDS = 10;
inline float const BOSS_YGG_HEALTH_RATIO = 0.5;
inline float const BOSS_DAMAGE_MULTIPLIER = 2;
inline float const BOSS_SIZE_MULTIPLIER = 5;
inline float const BOSS_SPAWN_CHANCE = 0.004;
inline uint32_t const MAX_BOSSES = 3;
// what a boss pays out: its score, and the drops of the mob it is: every chance is
// BOSS_DROP_CHANCE_MULTIPLIER times as big, and there is always one drop of Legendary or better
// (one of the mob's own, or else any Legendary or Mythic petal)
inline uint32_t const BOSS_SCORE_MULTIPLIER = 10;
inline float const BOSS_DROP_CHANCE_MULTIPLIER = 10;
// the minions of a boss (what it summons) alive at the same time
inline uint32_t const BOSS_MAX_MINIONS = 6;
// the Cactus boss swells from 1x to this many times its size and back, in this many seconds
inline float const BOSS_CACTUS_SWELL = 1.25;
inline float const BOSS_CACTUS_PERIOD_SECONDS = 12;
// the Digger boss: all around it, this much damage to every mob and flower once a second
inline float const BOSS_DIGGER_RADIATION_DAMAGE = 1;

// the shells of the Tank mob explode where they hit (a flower, a mob or a wall): everything
// of another team inside the circle takes this much, once
inline float const TANK_EXPLOSION_RADIUS = 90;
inline float const TANK_EXPLOSION_DAMAGE = 10;

// devStabOSC: the player it hits gets a timer, and when it runs out STAB_TNT_COUNT TNT
// explode on them (over STAB_BLAST_SECONDS), in a circle that follows them
inline uint32_t const STAB_COUNTDOWN_SECONDS = 10;
inline uint32_t const STAB_BLAST_SECONDS = 3;
inline uint32_t const STAB_TNT_COUNT = 1000;
inline float const STAB_TNT_DAMAGE = 1;
inline float const STAB_RADIUS = 260;

inline float arena_width_of(uint8_t gamemode) {
    return gamemode == Gamemode::kSandbox ? SANDBOX_WIDTH : ARENA_WIDTH;
}
inline float arena_height_of(uint8_t gamemode) {
    return gamemode == Gamemode::kSandbox ? SANDBOX_HEIGHT : ARENA_HEIGHT;
}

namespace PetalID {
    typedef uint8_t T;
    enum : T {
        kNone,
        kBasic,
        kLight,
        kHeavy,
        kStinger,
        kLeaf,
        kTwin,
        kRose,
        kIris,
        kMissile,
        kDandelion,
        kBubble,
        kFaster,
        kRock,
        kCactus,
        kWeb,
        kWing,
        kPeas,
        kSand,
        kPincer,
        kDahlia,
        kTriplet,
        kAntEgg,
        kBlueIris,
        kPollen,
        kPoisonPeas,
        kBeetleEgg,
        kAzalea,
        kStick,
        kTringer,
        kTriweb,
        kAntennae,
        kTricac,
        kHeaviest,
        kThirdEye,
        kObserver,
        kPoisonCactus,
        kSalt,
        kUniqueBasic,
        kSquare,
        kMoon,
        kLotus,
        kCutter,
        kYinYang,
        kYggdrasil,
        kRice,
        kBone,
        kYucca,
        kCorn,
        kGuidedMissile,
        kHomingMissile,
        kHornetEgg,
        kMassiveBeetleEgg,
        kGoldenLeaf,
        kLightning,
        kOldLightning,
        kUranium,
        kPinger,
        kLargeWeb,
        kSoil,
        kPowder,
        kShell,
        kSponge,
        kHoney,
        kMagnet,
        kButterfly,
        kMythicIris,
        kMythicSponge,
        kFastest,
        kLegendaryDandelion,
        kClover,
        kDevStabOSC,
        kNumPetals
    };
};

namespace MobID {
    typedef uint8_t T;
    enum : T {
        kBabyAnt,
        kWorkerAnt,
        kSoldierAnt,
        kBee,
        kLadybug,
        kBeetle,
        kMassiveLadybug,
        kMassiveBeetle,
        kDarkLadybug,
        kHornet,
        kCactus,
        kRock,
        kBoulder,
        kCentipede,
        kEvilCentipede,
        kDesertCentipede,
        kSandstorm,
        kScorpion,
        kSpider,
        kAntHole,
        kQueenAnt,
        kShinyLadybug,
        kSquare,
        kDigger,
        kFireAnt,
        kAntBurrow,
        kMountain,
        kBumblebee,
        kTank,
        kNumMobs
    };
};

namespace RarityID {
    enum {
        kCommon,
        kUnusual,
        kRare,
        kEpic,
        kLegendary,
        kMythic,
        kUnique,
        kNumRarities
    };
};

// Drop luck: the clovers of the players in a biome (a zone of the map) add up to its drop
// luck. It does not build up over time: it is the same for as long as the same clovers are
// there, and it changes the moment one is added or taken off.
// A biome's drop luck stops adding up at MAX_ZONE_LUCK points
inline float const MAX_ZONE_LUCK = 24;
// how much more likely every drop is at full luck (a point of luck is a twentyfourth of this)
inline float const LUCK_DROP_BONUS = 0.6;

namespace ColorID {
    enum {
        kYellow,
        kGray,
        kBlue,
        kRed,
        kGreen,
        kPurple,
        kOrange,
        kNumColors
    };
};

namespace AIState {
    enum {
        kIdle,
        kIdleMoving,
        kReturning,
        kBasicAggro
    };
};

namespace AnimationType {
    enum {
        kLightning,
        kUranium,
        kExplosion
    };
};

namespace EntityFlags {
    enum {
        kIsDespawning,
        kNoFriendlyCollision,
        kDieOnParentDeath,
        kSpawnedFromZone,
        kNoDrops,
        kHasCulling,
        kIsCulled,
        kZombie,
        kCPUControlled
    };
};

namespace FaceFlags {
    enum {
        kAttacking,
        kDefending,
        kPoisoned,
        kDandelioned,
        kHoneyed,
        kDeadEyes,
        kSquareEyes,
        kDeveloper
    };
};

namespace EquipmentFlags {
    enum {
        kThirdEye,
        kAntennae,
        kObserver,
        kCutter,
        kNone
    };
};

namespace InputFlags {
    enum {
        kAttacking,
        kDefending
    };
}

namespace PetalFlags {
    enum {
        kLockedOn,
        kSplitProjectile,
        // a shell of the Tank mob: a circle that explodes on whatever it hits
        kTankShell
    };
};

namespace SettingFlags {
    enum {
        kAutoDelete
    };
};

struct PoisonDamage {
    float damage;
    float time;
};

struct PetalAttributes {
    enum {
        kPassiveRot,
        kNoRot,
        kFollowRot
    };
    float clump_radius = 0;
    float secondary_reload = 0;
    float constant_heal = 0;
    float burst_heal = 0;
    float mass = 0.1;
    float armor = 0;
    float poison_armor = 0;
    float dandelion_inflict_seconds = 0;
    float slow_inflict_seconds = 0;
    float vision_factor = 1;
    float extra_body_damage = 0;
    float extra_rotation_speed = 0;
    float extra_range = 0;
    float extra_health = 0;
    float damage_reflection = 0;
    float extra_damage_factor = 1;
    float extra_reload_factor = 1;
    float radius = 0;
    float extra_radius = 0;
    float speed_factor = 1;
    float health_factor = 1;
    float burst_shield = 0;
    float constant_sheild = 0;
    float period = 0;
    float pickup_range = 0;
    struct PoisonDamage poison_damage;
    uint8_t defend_only = 0;
    float icon_angle = 0;
    uint8_t split_projectile = 0;
    uint8_t rotation_style = kPassiveRot;
    uint8_t spawns = MobID::kNumMobs;
    uint8_t spawn_count = 0;
    uint8_t equipment = EquipmentFlags::kNone;
    uint8_t bounces = 0;
    uint8_t no_collision = 0;
    // the drop luck the petal adds to the biome of its flower (see MAX_ZONE_LUCK)
    float drop_luck = 0;
};

struct PetalData {
    char const *name;
    char const *description;
    float health;
    float damage;
    float radius;
    float reload;
    uint8_t count;
    uint8_t rarity;
    struct PetalAttributes attributes;
};

struct MobAttributes {
    float aggro_radius = 500;
    uint8_t segments = 1;
    uint8_t stationary;
    uint8_t hole;
    struct PoisonDamage poison_damage;
    // taken off every hit it takes (like the armor of the Bone petal)
    float armor = 0;
};

struct MobData {
    char const *name;
    char const *description;
    uint8_t rarity;
    RangeValue health;
    float damage;
    RangeValue radius;
    uint32_t xp;
    StaticArray<PetalID::T, MAX_DROPS_PER_MOB> drops;
    struct MobAttributes attributes;
};

struct SpawnChance {
    MobID::T id;
    float chance;
};

struct ZoneDefinition {
    float left;
    float top;
    float right;
    float bottom;
    float density;
    float drop_multiplier;
    StaticArray<struct SpawnChance, MobID::kNumMobs> spawns;
    uint32_t difficulty;
    uint32_t color;
    char const *name;
};

struct PlayerProgress {
    uint64_t restore_id;
    uint32_t score;
    std::array<PetalID::T, 2 * MAX_SLOT_COUNT> loadout;
};
