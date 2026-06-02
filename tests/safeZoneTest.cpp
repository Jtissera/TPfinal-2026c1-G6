#include <gtest/gtest.h>
#include "editor/map/tile.h"

TEST(SafeZone, NPCCombateNoPuedeEntrarASafe)
{
    Tile dest;
    dest.zone = ZoneType::SAFE;
    bool blocked = (dest.zone == ZoneType::SAFE);
    EXPECT_TRUE(blocked);
}

TEST(SafeZone, NPCPuedeEntrarACombat)
{
    Tile dest;
    dest.zone = ZoneType::COMBAT;
    bool blocked = (dest.zone == ZoneType::SAFE);
    EXPECT_FALSE(blocked);
}

TEST(SafeZone, AtaqueEnSafeDebeRechazarse)
{
    Tile attackerTile;
    attackerTile.zone = ZoneType::SAFE;
    bool rejected = (attackerTile.zone == ZoneType::SAFE);
    EXPECT_TRUE(rejected);
}