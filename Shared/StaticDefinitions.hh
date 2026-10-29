#pragma once

#include <Helpers/Array.hh>
#include <Helpers/Math.hh>

#include <cstdint>

inline uint32_t const MAX_LEVEL = 99;
inline uint32_t const TPS = 20;
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
inline uint32_t const MAX_SPONGE_PERIOD = 10 * TPS;

inline uint32_t const MAX_NAME_LENGTH = 16;
inline uint32_t const MAX_CHAT_LENGTH = 64;
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
        kNumGamemodes
    };
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

namespace ColorID {
    enum {
        kYellow,
        kGray,
        kBlue,
        kRed,
        kGreen,
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
        kUranium
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
        kSplitProjectile
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
