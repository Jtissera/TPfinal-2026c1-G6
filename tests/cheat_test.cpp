#include "testHelpers.h"
#include <gtest/gtest.h>

// ---------------------------------------------------------------------------
// INFINITE_HP
// ---------------------------------------------------------------------------

TEST(CheatInfiniteHp, ToggleOnSetsFlag) {
  auto p = makePlayer(1, 0, 0);
  p.toggleInfiniteHp();
  EXPECT_TRUE(p.hasInfiniteHp());
}

TEST(CheatInfiniteHp, ToggleOffClearsFlag) {
  auto p = makePlayer(1, 0, 0);
  p.toggleInfiniteHp();
  p.toggleInfiniteHp();
  EXPECT_FALSE(p.hasInfiniteHp());
}

TEST(CheatInfiniteHp, ActivationRestoresFullHp) {
  auto p = makePlayer(1, 0, 0, 40);
  ASSERT_EQ(p.getHp(), 40);

  p.toggleInfiniteHp();

  EXPECT_EQ(p.getHp(), p.getMaxHp());
}

TEST(CheatInfiniteHp, DamageIgnoredWhileActive) {
  auto p = makePlayer(1, 0, 0);
  p.toggleInfiniteHp();
  int16_t hpBefore = p.getHp();

  p.takeDamage(9999);

  EXPECT_EQ(p.getHp(), hpBefore);
  EXPECT_TRUE(p.isAlive());
}

TEST(CheatInfiniteHp, DamageAppliedAfterDeactivation) {
  auto p = makePlayer(1, 0, 0);
  p.toggleInfiniteHp();
  p.toggleInfiniteHp();

  p.takeDamage(10);

  EXPECT_EQ(p.getHp(), 90);
}

TEST(CheatInfiniteHp, PlayerCanDieNormallyWithoutCheat) {
  auto p = makePlayer(1, 0, 0);
  p.takeDamage(100);
  EXPECT_FALSE(p.isAlive());
}

// ---------------------------------------------------------------------------
// INFINITE_MANA
// ---------------------------------------------------------------------------

TEST(CheatInfiniteMana, ToggleOnSetsFlag) {
  auto p = makeMagePlayer(1, 0, 0);
  p.toggleInfiniteMana();
  EXPECT_TRUE(p.hasInfiniteMana());
}

TEST(CheatInfiniteMana, ToggleOffClearsFlag) {
  auto p = makeMagePlayer(1, 0, 0);
  p.toggleInfiniteMana();
  p.toggleInfiniteMana();
  EXPECT_FALSE(p.hasInfiniteMana());
}

TEST(CheatInfiniteMana, ActivationFillsMana) {
  auto p = makeMagePlayer(1, 0, 0);
  p.spendMana(50);
  ASSERT_LT(p.getMana(), p.getMaxMana());

  p.toggleInfiniteMana();

  EXPECT_EQ(p.getMana(), p.getMaxMana());
}

TEST(CheatInfiniteMana, SpendManaSucceedsWithoutDeducting) {
  auto p = makeMagePlayer(1, 0, 0);
  p.toggleInfiniteMana();
  int16_t manaBefore = p.getMana();

  bool ok = p.spendMana(9999);

  EXPECT_TRUE(ok);
  EXPECT_EQ(p.getMana(), manaBefore);
}

TEST(CheatInfiniteMana, SpendManaFailsNormallyAfterDeactivation) {
  auto p = makeMagePlayer(1, 0, 0);
  p.toggleInfiniteMana();
  p.toggleInfiniteMana();

  // gastar todo el mana
  p.spendMana(p.getMana());
  ASSERT_EQ(p.getMana(), 0);

  EXPECT_FALSE(p.spendMana(1));
}

TEST(CheatInfiniteMana, IgnoredForClassWithoutMagic) {
  // makePlayer usa warrior (canUseMagic = false)
  auto p = makePlayer(1, 0, 0);
  p.toggleInfiniteMana();

  EXPECT_FALSE(p.spendMana(1));
  EXPECT_EQ(p.getMana(), 0);
}

TEST(CheatInfiniteMana, TickDoesNotRestoreManaWhileActive) {
  auto p = makeMagePlayer(1, 0, 0);
  p.toggleInfiniteMana(); // llena y activa
  int16_t manaFull = p.getMana();

  p.tick(0.0f, 50.0f);

  EXPECT_EQ(p.getMana(), manaFull);
}

// ---------------------------------------------------------------------------
// DIE cheat  (lógica de Player::die directamente)
// ---------------------------------------------------------------------------

TEST(CheatDie, ForcesDeadState) {
  auto p = makePlayer(1, 0, 0);
  p.die(0);

  EXPECT_TRUE(p.isGhost());
  EXPECT_EQ(p.getHp(), 0);
}

TEST(CheatDie, DropsExcessGold) {
  auto p = makePlayer(1, 0, 0);
  p.addGold(500);

  uint32_t excess = p.die(100);

  EXPECT_EQ(excess, 400);
  EXPECT_EQ(p.getGold(), 100);
}

TEST(CheatDie, NoEffectWhenAlreadyDead) {
  auto p = makePlayer(1, 0, 0);
  p.die(0);

  uint32_t excess = p.die(0);

  EXPECT_EQ(excess, 0);
  EXPECT_TRUE(p.isGhost());
}

TEST(CheatDie, BypassesInfiniteHpFlag) {
  // DIE llama die() directamente, no takeDamage(), asi que el flag no aplica
  auto p = makePlayer(1, 0, 0);
  p.toggleInfiniteHp();

  p.die(0);

  EXPECT_TRUE(p.isGhost());
}

TEST(CheatDie, PurgesInventoryOnDeath) {
  auto p = makePlayer(1, 0, 0);
  p.getInventory().addItem(makeWeapon(5, 10));

  p.die(0);
  auto dropped = p.purgeInventoryOnDeath();

  EXPECT_EQ(dropped.size(), 1u);
}

// ---------------------------------------------------------------------------
// Combinaciones
// ---------------------------------------------------------------------------

TEST(CheatCombined, BothFlagsToggleIndependently) {
  auto p = makeMagePlayer(1, 0, 0);
  p.toggleInfiniteHp();
  p.toggleInfiniteMana();

  EXPECT_TRUE(p.hasInfiniteHp());
  EXPECT_TRUE(p.hasInfiniteMana());

  p.toggleInfiniteHp();

  EXPECT_FALSE(p.hasInfiniteHp());
  EXPECT_TRUE(p.hasInfiniteMana());
}

TEST(CheatCombined, InfiniteHpAndManaActiveSimultaneously) {
  auto p = makeMagePlayer(1, 0, 0);
  p.toggleInfiniteHp();
  p.toggleInfiniteMana();

  p.takeDamage(9999);
  bool ok = p.spendMana(9999);

  EXPECT_TRUE(p.isAlive());
  EXPECT_TRUE(ok);
  EXPECT_EQ(p.getMana(), p.getMaxMana());
}