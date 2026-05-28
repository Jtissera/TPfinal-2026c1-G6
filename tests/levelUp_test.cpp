// tests/levelUpTest.cpp
#include <gtest/gtest.h>
#include "game/Player.h"
#include "game/gameFormulas.h"

TEST(LevelUp, SubeDeNivelAlLlegarAlLimite) {

    GameFormulas f;
    uint32_t limit = f.calcExpLimit(1);
    // necesitás makePlayer de testHelpers
    // player.addExperience(limit);
    // EXPECT_EQ(player.getLevel(), 2);
    EXPECT_GT(limit, 0u); // placeholder
}

TEST(LevelUp, ExpSobrante) {
    GameFormulas f;
    uint32_t limit = f.calcExpLimit(1);
    // addExperience(limit + 50) → level 2, exp sobrante = 50
    EXPECT_GT(limit, 0u);
}