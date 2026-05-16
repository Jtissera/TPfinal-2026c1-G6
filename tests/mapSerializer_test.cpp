#include "gtest/gtest.h"

#include <filesystem>
#include <stdexcept>
#include <fstream>

#include "editor/map/mapData.h"
#include "editor/map/mapSerializer.h"
#include "editor/map/tile.h"

namespace {

// Ruta temporal para los tests
static const std::string TMP_PATH = "/tmp/test_argentum_map.argmap";

class MapSerializerTest : public ::testing::Test {
protected:
    void TearDown() override {
        // Limpiar archivo temporal después de cada test
        std::filesystem::remove(TMP_PATH);
    }
};

// -------------------------------- Guardado y carga básica --------------------------------

TEST_F(MapSerializerTest, SaveAndLoadPreservesDimensions) {
    MapData map(15, 8);
    MapSerializer::save(map, TMP_PATH);

    MapData loaded = MapSerializer::load(TMP_PATH);
    EXPECT_EQ(loaded.width(),  15);
    EXPECT_EQ(loaded.height(), 8);
}

TEST_F(MapSerializerTest, SaveAndLoadPreservesName) {
    MapData map(10, 10);
    map.setName("caverna_oscura");
    MapSerializer::save(map, TMP_PATH);

    MapData loaded = MapSerializer::load(TMP_PATH);
    EXPECT_EQ(loaded.name(), "caverna_oscura");
}

// -------------------------------- Preservacion de tiles --------------------------------

TEST_F(MapSerializerTest, SaveAndLoadPreservesTileType) {
    MapData map(5, 5);
    map.at(2, 3).type = TileType::WATER;
    MapSerializer::save(map, TMP_PATH);

    MapData loaded = MapSerializer::load(TMP_PATH);
    EXPECT_EQ(loaded.at(2, 3).type, TileType::WATER);
}

TEST_F(MapSerializerTest, SaveAndLoadPreservesZoneType) {
    MapData map(5, 5);
    map.at(0, 0).zone = ZoneType::COMBAT;
    MapSerializer::save(map, TMP_PATH);

    MapData loaded = MapSerializer::load(TMP_PATH);
    EXPECT_EQ(loaded.at(0, 0).zone, ZoneType::COMBAT);
}

TEST_F(MapSerializerTest, SaveAndLoadPreservesWalkable) {
    MapData map(5, 5);
    map.at(1, 1).walkable = false;
    MapSerializer::save(map, TMP_PATH);

    MapData loaded = MapSerializer::load(TMP_PATH);
    EXPECT_FALSE(loaded.at(1, 1).walkable);
}

TEST_F(MapSerializerTest, SaveAndLoadPreservesNpcId) {
    MapData map(5, 5);
    map.at(4, 4).npcId = 42;
    MapSerializer::save(map, TMP_PATH);

    MapData loaded = MapSerializer::load(TMP_PATH);
    EXPECT_EQ(loaded.at(4, 4).npcId, 42);
}

TEST_F(MapSerializerTest, AllTileTypesRoundtrip) {
    MapData map(5, 1);
    map.at(0, 0).type = TileType::GRASS;
    map.at(1, 0).type = TileType::WATER;
    map.at(2, 0).type = TileType::WALL;
    map.at(3, 0).type = TileType::FLOOR;
    map.at(4, 0).type = TileType::DOOR;
    MapSerializer::save(map, TMP_PATH);

    MapData loaded = MapSerializer::load(TMP_PATH);
    EXPECT_EQ(loaded.at(0, 0).type, TileType::GRASS);
    EXPECT_EQ(loaded.at(1, 0).type, TileType::WATER);
    EXPECT_EQ(loaded.at(2, 0).type, TileType::WALL);
    EXPECT_EQ(loaded.at(3, 0).type, TileType::FLOOR);
    EXPECT_EQ(loaded.at(4, 0).type, TileType::DOOR);
}

// -------------------------------- Casos de error --------------------------------

TEST_F(MapSerializerTest, LoadNonExistentFileThrows) {
    EXPECT_THROW(
        MapSerializer::load("/tmp/no_existe_este_archivo.argmap"),
        std::runtime_error
    );
}

TEST_F(MapSerializerTest, LoadCorruptFileThrows) {
    // Escribir basura
    std::ofstream f(TMP_PATH, std::ios::binary);
    f << "esto no es un mapa valido!!!";
    f.close();

    EXPECT_THROW(MapSerializer::load(TMP_PATH), std::runtime_error);
}

TEST_F(MapSerializerTest, SaveToInvalidPathThrows) {
    MapData map(5, 5);
    EXPECT_THROW(
        MapSerializer::save(map, "/ruta/que/no/existe/mapa.argmap"),
        std::runtime_error
    );
}

// -------------------------------- Mapa completo --------------------------------

TEST_F(MapSerializerTest, LargeMapRoundtrip) {
    MapData map(100, 100);
    for (uint16_t x = 0; x < 100; x += 2)
        map.at(x, x < 100 ? x : 99).type = TileType::WALL;

    MapSerializer::save(map, TMP_PATH);
    MapData loaded = MapSerializer::load(TMP_PATH);

    EXPECT_EQ(loaded.width(),  100);
    EXPECT_EQ(loaded.height(), 100);
    for (uint16_t x = 0; x < 100; x += 2)
        EXPECT_EQ(loaded.at(x, x < 100 ? x : 99).type, TileType::WALL);
}

}
