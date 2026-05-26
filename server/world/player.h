#pragma once
#include <cstdint>

struct player {
    uint32_t id;
    int x, y;

    static constexpr int SPEED = 10;

// Player.h — todo x2
static constexpr int SPRITE_W = 128;
static constexpr int SPRITE_H = 128;
static constexpr int HITBOX_W = 64;
static constexpr int HITBOX_H = 96;
static constexpr int HITBOX_OFFSET_X = 32;
static constexpr int HITBOX_OFFSET_Y = 32;
};