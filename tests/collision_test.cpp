#include <gtest/gtest.h>
#include "server/world/gameWorld.h"
#include "server/world/CollisionSystem.h"
#include "server/world/Hitbox.h"
#include "server/game/Player.h"
#include "editor/map/mapData.h"
#include "editor/map/tile.h"

// ─── Constantes del dominio ───────────────────────────────────────
static constexpr int TILE_SIZE = 96;
static constexpr int SPEED     = 10;

// Hitbox estándar del jugador — igual a la que usa Player internamente
static constexpr Hitbox PLAYER_HITBOX{32, 32, 64, 96};

// ─── Helpers ─────────────────────────────────────────────────────

static Player makePlayer(uint32_t id, int x, int y) {
    RaceStats race{};
    race.health = 1.0f; race.mana = 1.0f; race.recovery = 0.0f;
    race.constitution = 10; race.intelligence = 10;
    race.strength = 10;     race.agility = 10;

    ClassStats cls{};
    cls.health = 1.0f; cls.mana = 1.0f;
    cls.meditation = 0.0f; cls.canUseMagic = false;

    Player p(id, "test", race, cls, 100, 0);
    p.setPos(x, y);
    return p;
}

static MapData makeMap(int w, int h) {
    return MapData(w, h);
}

static void blockTile(MapData& m, int tx, int ty) {
    m.at(tx, ty).walkable = false;
}

// Posición X tal que moverse RIGHT toca el tile (targetTileX, *)
static int spawnBeforeRightBlock(int targetTileX) {
    // right(nx + SPEED) = nx + SPEED + PLAYER_HITBOX.offsetX + PLAYER_HITBOX.width - 1
    // Para que toque targetTileX: (right / TILE_SIZE) == targetTileX
    // nx = targetTileX * TILE_SIZE - PLAYER_HITBOX.offsetX - PLAYER_HITBOX.width - SPEED + 1
    return targetTileX * TILE_SIZE
         - PLAYER_HITBOX.offsetX
         - PLAYER_HITBOX.width
         - SPEED + 1;
}

// ─── CollisionSystem unit tests ───────────────────────────────────

TEST(CollisionSystemTest, OpenMapDoesNotCollide) {
    MapData map = makeMap(10, 10);
    CollisionSystem cs(map);
    EXPECT_FALSE(cs.wouldCollide(5 * TILE_SIZE, 5 * TILE_SIZE, PLAYER_HITBOX));
}

TEST(CollisionSystemTest, BlockedTileCollides) {
    MapData map = makeMap(10, 10);
    blockTile(map, 5, 5);
    CollisionSystem cs(map);
    // Posicionar hitbox exactamente sobre el tile bloqueado
    int x = 5 * TILE_SIZE - PLAYER_HITBOX.offsetX;
    int y = 5 * TILE_SIZE - PLAYER_HITBOX.offsetY;
    EXPECT_TRUE(cs.wouldCollide(x, y, PLAYER_HITBOX));
}

TEST(CollisionSystemTest, InBoundsCenter) {
    MapData map = makeMap(10, 10);
    CollisionSystem cs(map);
    EXPECT_TRUE(cs.isInBounds(5 * TILE_SIZE, 5 * TILE_SIZE, PLAYER_HITBOX));
}

TEST(CollisionSystemTest, OutOfBoundsLeft) {
    MapData map = makeMap(10, 10);
    CollisionSystem cs(map);
    int x = -PLAYER_HITBOX.offsetX - 1;
    EXPECT_FALSE(cs.isInBounds(x, 5 * TILE_SIZE, PLAYER_HITBOX));
}

TEST(CollisionSystemTest, OutOfBoundsTop) {
    MapData map = makeMap(10, 10);
    CollisionSystem cs(map);
    int y = -PLAYER_HITBOX.offsetY - 1;
    EXPECT_FALSE(cs.isInBounds(5 * TILE_SIZE, y, PLAYER_HITBOX));
}

TEST(CollisionSystemTest, HitboxSpansTwoTilesOnlyBottomBlocked) {
    MapData map = makeMap(10, 10);
    blockTile(map, 5, 6);
    CollisionSystem cs(map);
    // Hitbox con top en tile 5 y bottom en tile 6
    int y = 5 * TILE_SIZE + TILE_SIZE - PLAYER_HITBOX.offsetY - 1;
    EXPECT_TRUE(cs.wouldCollide(5 * TILE_SIZE, y, PLAYER_HITBOX));
}

// ─── GameWorld movement tests ─────────────────────────────────────

TEST(CollisionTest, MoveRightOnOpenMap) {
    GameWorld world(makeMap(20, 20));
    world.addPlayer(makePlayer(1, 5 * TILE_SIZE, 5 * TILE_SIZE));
    EXPECT_TRUE(world.movePlayer(1, Direction::RIGHT));
}

TEST(CollisionTest, MoveLeftOnOpenMap) {
    GameWorld world(makeMap(20, 20));
    world.addPlayer(makePlayer(1, 5 * TILE_SIZE, 5 * TILE_SIZE));
    EXPECT_TRUE(world.movePlayer(1, Direction::LEFT));
}

TEST(CollisionTest, MoveUpOnOpenMap) {
    GameWorld world(makeMap(20, 20));
    world.addPlayer(makePlayer(1, 5 * TILE_SIZE, 5 * TILE_SIZE));
    EXPECT_TRUE(world.movePlayer(1, Direction::UP));
}

TEST(CollisionTest, MoveDownOnOpenMap) {
    GameWorld world(makeMap(20, 20));
    world.addPlayer(makePlayer(1, 5 * TILE_SIZE, 5 * TILE_SIZE));
    EXPECT_TRUE(world.movePlayer(1, Direction::DOWN));
}

TEST(CollisionTest, BlockedTileToTheRight) {
    MapData map = makeMap(20, 20);
    blockTile(map, 7, 5);
    GameWorld world(std::move(map));

    world.addPlayer(makePlayer(1, spawnBeforeRightBlock(7), 5 * TILE_SIZE));
    EXPECT_FALSE(world.movePlayer(1, Direction::RIGHT));
}

TEST(CollisionTest, BlockedTileToTheLeft) {
    MapData map = makeMap(20, 20);
    blockTile(map, 3, 5);
    GameWorld world(std::move(map));

    // left(nx - SPEED) toca tile 3
    int spawnX = 4 * TILE_SIZE - 1 + SPEED - PLAYER_HITBOX.offsetX;
    world.addPlayer(makePlayer(1, spawnX, 5 * TILE_SIZE));
    EXPECT_FALSE(world.movePlayer(1, Direction::LEFT));
}

TEST(CollisionTest, BlockedTileAbove) {
    MapData map = makeMap(20, 20);
    blockTile(map, 5, 3);
    GameWorld world(std::move(map));

    // top(ny - SPEED) toca tile 3
    int spawnY = 4 * TILE_SIZE - 1 + SPEED - PLAYER_HITBOX.offsetY;
    world.addPlayer(makePlayer(1, 5 * TILE_SIZE, spawnY));
    EXPECT_FALSE(world.movePlayer(1, Direction::UP));
}

TEST(CollisionTest, BlockedTileBelow) {
    MapData map = makeMap(20, 20);
    blockTile(map, 5, 8);
    GameWorld world(std::move(map));

    // bottom(ny + SPEED) toca tile 8
    int spawnY = 8 * TILE_SIZE
               - PLAYER_HITBOX.offsetY
               - PLAYER_HITBOX.height
               - SPEED + 1;
    world.addPlayer(makePlayer(1, 5 * TILE_SIZE, spawnY));
    EXPECT_FALSE(world.movePlayer(1, Direction::DOWN));
}

TEST(CollisionTest, CannotMoveLeftOfMap) {
    GameWorld world(makeMap(20, 20));
    int spawnX = SPEED -1- PLAYER_HITBOX.offsetX;
    world.addPlayer(makePlayer(1, spawnX, 5 * TILE_SIZE));
    EXPECT_FALSE(world.movePlayer(1, Direction::LEFT));
}

TEST(CollisionTest, CannotMoveAboveMap) {
    GameWorld world(makeMap(20, 20));
    int spawnY = SPEED -1 - PLAYER_HITBOX.offsetY;
    world.addPlayer(makePlayer(1, 5 * TILE_SIZE, spawnY));
    EXPECT_FALSE(world.movePlayer(1, Direction::UP));
}

TEST(CollisionTest, CannotMoveBeyondRightBound) {
    GameWorld world(makeMap(10, 10));
    int spawnX = 10 * TILE_SIZE
               - PLAYER_HITBOX.offsetX
               - PLAYER_HITBOX.width
               - SPEED;
    world.addPlayer(makePlayer(1, spawnX, 5 * TILE_SIZE));
    EXPECT_TRUE(world.movePlayer(1, Direction::RIGHT));   // último válido
    EXPECT_FALSE(world.movePlayer(1, Direction::RIGHT));  // sale del mapa
}

TEST(CollisionTest, CannotMoveBeyondBottomBound) {
    GameWorld world(makeMap(10, 10));
    int spawnY = 10 * TILE_SIZE
               - PLAYER_HITBOX.offsetY
               - PLAYER_HITBOX.height
               - SPEED;
    world.addPlayer(makePlayer(1, 5 * TILE_SIZE, spawnY));
    EXPECT_TRUE(world.movePlayer(1, Direction::DOWN));
    EXPECT_FALSE(world.movePlayer(1, Direction::DOWN));
}

TEST(CollisionTest, PositionUpdatedAfterMoveRight) {
    GameWorld world(makeMap(20, 20));
    int startX = 5 * TILE_SIZE;
    int startY = 5 * TILE_SIZE;
    world.addPlayer(makePlayer(1, startX, startY));

    world.movePlayer(1, Direction::RIGHT);

    EXPECT_EQ(world.getX(1), startX + SPEED);
    EXPECT_EQ(world.getY(1), startY);         
}

TEST(CollisionTest, PositionNotUpdatedOnBlocked) {
    MapData map = makeMap(20, 20);
    blockTile(map, 3, 5);
    GameWorld world(std::move(map));

    int startX = 3 * TILE_SIZE - PLAYER_HITBOX.offsetX + SPEED;
    int startY = 5 * TILE_SIZE;
    world.addPlayer(makePlayer(1, startX, startY));

    world.movePlayer(1, Direction::LEFT);

    EXPECT_EQ(world.getX(1), startX);
    EXPECT_EQ(world.getY(1), startY);
}

TEST(CollisionTest, RepeatedMovementDoesNotPassThroughWall) {
    MapData map = makeMap(20, 20);
    blockTile(map, 8, 5);
    GameWorld world(std::move(map));

    world.addPlayer(makePlayer(1, 5 * TILE_SIZE, 5 * TILE_SIZE));

    for (int i = 0; i < 100; ++i)
        world.movePlayer(1, Direction::RIGHT);

    // La hitbox no puede haber cruzado el tile bloqueado
    int maxX = 8 * TILE_SIZE - PLAYER_HITBOX.offsetX - PLAYER_HITBOX.width;
    EXPECT_LE(world.getX(1), maxX);
}

TEST(CollisionTest, MoveNonExistentPlayerReturnsFalse) {
    GameWorld world(makeMap(20, 20));
    EXPECT_FALSE(world.movePlayer(999, Direction::RIGHT));
}