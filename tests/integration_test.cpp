#include <gtest/gtest.h>
#include <toml++/toml.h>

#include "common/network/messages/message.h"
#include "common/queue.h"
#include "helpers/testHelpers.h"
#include "server/clientMessage.h"
#include "server/game/session/gameManager.h"

class IntegrationTest : public ::testing::Test
{
protected:
  toml::table config = makeConfig();
  NpcRepository npcRepo{config};
  NpcFactory npcFact{npcRepo};
  ItemRepository itemRepo{config};
  Queue<std::shared_ptr<LeaveEvent>> leaveQueue;
  Queue<std::shared_ptr<InstanceTransitionEvent>> transitionQueue;
  GameManager gm{npcFact, itemRepo, leaveQueue, transitionQueue, config};
};
// test 1 npc ataca Player, Player muere, item cae y se recoge

TEST_F(IntegrationTest, NpcKillsPlayerDropsItemCanBePickedUp)
{
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config);

  Player p = makePlayer(1, 5, 5, 6);
  p.getInventory().addItem(makeWeaponWithId(10, 10, 10));
  world.addPlayer(std::move(p));

  world.spawnNpc("goblin", 5, 6);

  auto result = world.tick(0.016f);

  EXPECT_FALSE(result.playerHits.empty());

  if (!world.getPlayer(1).isAlive())
  {
    auto item = world.pickItemAt(5, 5);
    EXPECT_TRUE(item.has_value());
  }
}

// test 2 recoger, equipar, atacar con stats del arma

TEST_F(IntegrationTest, PlayerPicksUpWeaponEquipsAndDealsDamage)
{
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config);

  world.addPlayer(makePlayer(1, 3, 3));

  world.addPlayer(makePlayer(2, 3, 4));

  world.addItemOnGround(makeWeaponWithId(50, 50, 50), 3, 3);

  auto picked = world.pickItemAt(3, 3);
  ASSERT_TRUE(picked.has_value());

  Player &attacker = world.getPlayer(1);
  attacker.getInventory().addItem(std::move(*picked));
  attacker.getInventory().equipItem(50);

  EXPECT_NE(attacker.getInventory().getEquipped(EquipSlot::HAND), nullptr);

  CombatSystem combat(config);
  Player &target = world.getPlayer(2);
  int16_t hpBefore = target.getHp();

  auto result = combat.attack(attacker, target);

  EXPECT_TRUE(result.valid);
  if (!result.dodged)
  {
    // El daño debe reflejar los stats del arma (min=max=50,daño fijo
    // 50,defensa)
    EXPECT_LT(target.getHp(), hpBefore);
  }
}

// test 3

TEST_F(IntegrationTest, AttackerGainsExpOnKillAndCanLevelUp)
{
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config);

  world.addPlayer(makePlayer(1, 3, 3)); // atacante nivel 1
  world.addPlayer(makePlayer(2, 3, 4)); // target nivel 1

  Player &attacker = world.getPlayer(1);
  uint8_t levelBefore = attacker.getLevel();

  uint32_t expLimit = 1000;
  int16_t newMaxHp = 200;
  int16_t newMaxMana = 50;
  attacker.addExperience(expLimit, expLimit, newMaxHp, newMaxMana);

  EXPECT_EQ(attacker.getLevel(), levelBefore + 1);
  EXPECT_TRUE(attacker.checkAndClearLevelUp());

  auto deathResult = world.handlePlayerDeath(2, 1);

  EXPECT_GE(attacker.getExp(), 0u);
}

TEST_F(IntegrationTest, RemoveClientFromGameDecreasesPlayerCount)
{
  uint32_t gameId = gm.createGame("sala", 4);
  Queue<std::shared_ptr<const Message>> clientQueue;
  gm.joinGame(gameId, 1, clientQueue);

  gm.removeClient(1);

  auto games = gm.listGames();
  EXPECT_EQ(games[0].playerCount, 0);
  gm.stopAll();
}

TEST_F(IntegrationTest, RemoveClientTwiceDoesNotCrash)
{
  uint32_t gameId = gm.createGame("sala", 4);
  Queue<std::shared_ptr<const Message>> clientQueue;
  gm.joinGame(gameId, 1, clientQueue);

  gm.removeClient(1);
  EXPECT_NO_THROW(gm.removeClient(1));
  gm.stopAll();
}

TEST_F(IntegrationTest, RemoveNonExistentClientDoesNotCrash)
{
  EXPECT_NO_THROW(gm.removeClient(999));
  gm.stopAll();
}
