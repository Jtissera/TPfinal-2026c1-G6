#pragma once

#include <cstdint>
#include <string>

struct ClassStats {
    std::string name;

    float health;
    float mana;
    float meditation;

    bool canUseMagic;
};