#pragma once
#include <cstdint>

struct Hitbox {
    int offsetX;  
    int offsetY;
    int width;
    int height;

    int left(int entityX)   const { return entityX + offsetX; }
    int top(int entityY)    const { return entityY + offsetY; }
    int right(int entityX)  const { return entityX + offsetX + width  - 1; }
    int bottom(int entityY) const { return entityY + offsetY + height - 1; }
};