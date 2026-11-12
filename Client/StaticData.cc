#include <Client/StaticData.hh>

#include <cmath>

std::array<uint32_t, RarityID::kNumRarities> const RARITY_COLORS = { 
    0xff7eef6d, 0xffffe65d, 0xff4d52e3, 
    0xff861fde, 0xffde1f1f, 0xff1fdbde,
    0xffde1f65
 }; // 0xffff2b75, 0xfff70fb6};

std::array<uint32_t, ColorID::kNumColors> const FLOWER_COLORS = {
    0xffffe763, 0xff999999, 0xff689ce2, 0xffec6869, 0xff55bb55
};

std::array<char const *, RarityID::kNumRarities> const RARITY_NAMES = {
    "Common",
    "Unusual",
    "Rare",
    "Epic",
    "Legendary",
    "Mythic",
    "Unique"
};

std::array<char const, MAX_SLOT_COUNT> const SLOT_KEYBINDS = 
    {'1','2','3','4','5','6','7','8','9','0','-','='};


uint32_t mob_track_color(MobID::T id) {
    // hues spread by the golden ratio so that mobs next to each other in the list differ
    float h = std::fmod(id * 0.618034f, 1.0f) * 6;
    float s = 0.85f, v = 1.0f;
    float c = v * s;
    float x = c * (1 - std::fabs(std::fmod(h, 2.0f) - 1));
    float m = v - c;
    float r = 0, g = 0, b = 0;
    if (h < 1) { r = c; g = x; }
    else if (h < 2) { r = x; g = c; }
    else if (h < 3) { g = c; b = x; }
    else if (h < 4) { g = x; b = c; }
    else if (h < 5) { r = x; b = c; }
    else { r = c; b = x; }
    return 0xff000000 | ((uint32_t) ((b + m) * 255) << 16) | ((uint32_t) ((g + m) * 255) << 8) | (uint32_t) ((r + m) * 255);
}
