#include "helpers/testHelpers.h"
#include <gtest/gtest.h>

TEST(CombatSystemTest, AttackReducesTargetHp)
{
  CombatSystem combat(makeConfig());
  Player attacker = makePlayer(1, 0, 0);
  Player target = makePlayer(2, 0, 1);

  attacker.getInventory().addItem(makeWeapon(10, 10));
  attacker.getInventory().equipItem(1);

  int16_t hpBefore = target.getHp();
  auto result = combat.attack(attacker, target);

  EXPECT_TRUE(result.valid);
  if (!result.dodged)
  {
    EXPECT_LT(target.getHp(), hpBefore);
  }
}

TEST(CombatSystemTest, AttackOutOfRangeIsInvalid)
{
  CombatSystem combat(makeConfig());
  Player attacker = makePlayer(1, 0, 0);
  Player target = makePlayer(2, 5, 5);

  attacker.getInventory().addItem(makeWeapon(10, 10));
  attacker.getInventory().equipItem(1);

  auto result = combat.attack(attacker, target);
  EXPECT_FALSE(result.valid);
}

TEST(CombatSystemTest, RangedAttackReachesDistantTarget)
{
  CombatSystem combat(makeConfig());
  Player attacker = makePlayer(1, 0, 0);
  Player target = makePlayer(2, 8, 0);

  attacker.getInventory().addItem(makeWeapon(5, 5, true));
  attacker.getInventory().equipItem(1);

  auto result = combat.attack(attacker, target);
  EXPECT_TRUE(result.valid);
}

TEST(CombatSystemTest, DeadAttackerCannotAttack)
{
  CombatSystem combat(makeConfig());
  Player attacker = makePlayer(1, 0, 0, 1);
  Player target = makePlayer(2, 0, 1);

  attacker.die(0);
  auto result = combat.attack(attacker, target);
  EXPECT_FALSE(result.valid);
}

TEST(CombatSystemTest, DeadTargetCannotBeAttacked)
{
  CombatSystem combat(makeConfig());
  Player attacker = makePlayer(1, 0, 0);
  Player target = makePlayer(2, 0, 1, 1);

  target.die(0);
  auto result = combat.attack(attacker, target);
  EXPECT_FALSE(result.valid);
}

TEST(CombatSystemTest, ArmorReducesDamage)
{
  CombatSystem combat(makeConfig());
  Player attacker = makePlayer(1, 0, 0);
  Player noArmor = makePlayer(2, 0, 1);
  Player withArmor = makePlayer(3, 0, 1);

  attacker.getInventory().addItem(makeWeapon(20, 20));
  attacker.getInventory().equipItem(1);

  withArmor.getInventory().addItem(makeArmor(10, 10));
  withArmor.getInventory().equipItem(2);

  auto r1 = combat.attack(attacker, noArmor);

  Player attacker2 = makePlayer(4, 0, 0);
  attacker2.getInventory().addItem(makeWeapon(20, 20));
  attacker2.getInventory().equipItem(1);
  auto r2 = combat.attack(attacker2, withArmor);

  if (r1.valid && !r1.dodged && r2.valid && !r2.dodged)
  {
    EXPECT_GE(r1.damage, r2.damage);
  }
}

TEST(CombatSystemTest, NpcAttacksPlayer)
{
  CombatSystem combat(makeConfig());
  static NpcStats stats = makeNpcStats(50, 10, 10);
  Npc npc(1, stats, 0, 0);
  Player target = makePlayer(2, 0, 1);

  auto result = combat.attack(npc, target);
  EXPECT_TRUE(result.valid);
}

TEST(CombatSystemTest, PlayerAttacksNpc)
{
  CombatSystem combat(makeConfig());
  Player attacker = makePlayer(1, 0, 0);
  static NpcStats stats = makeNpcStats(50, 1, 2);
  Npc npc(2, stats, 0, 1);

  attacker.getInventory().addItem(makeWeapon(10, 10));
  attacker.getInventory().equipItem(1);

  auto result = combat.attack(attacker, npc);
  EXPECT_TRUE(result.valid);
}

TEST(CombatSystemTest, PvPRestrictedBelowLevel12)
{
  CombatSystem combat(makeConfig());
  Player attacker = makePlayer(1, 0, 0);
  Player target = makePlayer(2, 0, 1);

  auto world = makeTestWorld();

  attacker.getInventory().addItem(makeWeapon(10, 10));
  attacker.getInventory().equipItem(1);

  auto result = combat.attackPlayer(attacker, target, *world);
  EXPECT_FALSE(result.valid);
}