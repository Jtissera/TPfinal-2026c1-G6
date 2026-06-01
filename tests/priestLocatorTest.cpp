#include <gtest/gtest.h>
#include "server/resurrection/priestLocator.h"
#include "editor/map/mapData.h"
#include "editor/map/tile.h"
#include "common/npcType.h"

static MapData makeMap(uint16_t w, uint16_t h)
{
    return MapData(w, h);
}

TEST(PriestLocator, RetornaNulloptSinSacerdote)
{
    MapData map = makeMap(5, 5);
    auto result = PriestLocator::findNearest(map, 2, 2);
    EXPECT_FALSE(result.has_value());
}

TEST(PriestLocator, EncuentraUnicoSacerdote)
{
    MapData map = makeMap(5, 5);
    map.at(3, 3).npc = NpcType::PRIEST;
    auto result = PriestLocator::findNearest(map, 0, 0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->first, 3);
    EXPECT_EQ(result->second, 3);
}

TEST(PriestLocator, EncuentraMasCercanoEntreVarios)
{
    MapData map = makeMap(10, 10);
    map.at(8, 8).npc = NpcType::PRIEST; // lejos
    map.at(2, 2).npc = NpcType::PRIEST; // cerca
    auto result = PriestLocator::findNearest(map, 1, 1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->first, 2);
    EXPECT_EQ(result->second, 2);
}

TEST(PriestLocator, SacerdoteEnMismaTile)
{
    MapData map = makeMap(5, 5);
    map.at(2, 2).npc = NpcType::PRIEST;
    auto result = PriestLocator::findNearest(map, 2, 2);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->first, 2);
    EXPECT_EQ(result->second, 2);
}