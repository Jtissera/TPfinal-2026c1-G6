#include "../server/game/combat/itemEffectHandler.h"
#include "helpers/testHelpers.h"
#include <gtest/gtest.h>

// ─── Level up ────────────────────────────────────────────────────────────────

TEST(PlayerTest, LevelUpOnExpLimit) {
  static auto race = makeRace();
  static auto cls = makeClass();
  static auto config = makeConfig();
  Player p(1, "test", race, cls, 100, 50, config);

  p.addExperience(1000, 1000, 200, 50);

  EXPECT_EQ(p.getLevel(), 2);
  EXPECT_TRUE(p.checkAndClearLevelUp());
  EXPECT_FALSE(p.checkAndClearLevelUp());
}

TEST(PlayerTest, LevelUpIncreasesMaxHp) {
  static auto race = makeRace();
  static auto cls = makeClass();
  static auto config = makeConfig();
  Player p(1, "test", race, cls, 100, 50, config);

  p.addExperience(1000, 1000, 200, 50);
  EXPECT_EQ(p.getMaxHp(), 200);
  EXPECT_EQ(p.getHp(), 200);
}

TEST(PlayerTest, NoLevelUpBelowLimit) {
  static auto race = makeRace();
  static auto cls = makeClass();
  static auto config = makeConfig();
  Player p(1, "test", race, cls, 100, 50, config);

  p.addExperience(500, 1000, 200, 50);
  EXPECT_EQ(p.getLevel(), 1);
  EXPECT_FALSE(p.checkAndClearLevelUp());
}

TEST(PlayerTest, ResurrectionRestoresHalfHp) {
  static auto race = makeRace();
  static auto cls = makeClass();
  static auto config = makeConfig();
  Player p(1, "test", race, cls, 100, 50, config);

  p.die(100);
  p.resurrect(0, 0);

  EXPECT_EQ(p.getHp(), 50);
  EXPECT_EQ(p.getMana(), 0);
  EXPECT_TRUE(p.isAlive());
}

TEST(PlayerTest, WarriorCannotRestoreMana) {
  static auto race = makeRace();
  static auto cls = makeClass();
  static auto config = makeConfig();
  Player p(1, "warrior", race, cls, 100, 0, config);

  p.restoreMana(50);
  EXPECT_EQ(p.getMana(), 0);
}

TEST(PlayerTest, WarriorCannotSpendMana) {
  static auto race = makeRace();
  static auto cls = makeClass();
  static auto config = makeConfig();
  Player p(1, "warrior", race, cls, 100, 0, config);

  EXPECT_FALSE(p.spendMana(10));
}

TEST(PlayerTest, WarriorManaPotionHasNoEffect) {
  ItemEffectHandler handler;
  static auto race = makeRace();
  static auto cls = makeClass();
  static auto config = makeConfig();
  Player p(1, "warrior", race, cls, 100, 0, config);

  Item potion = makePotion(1, 0, 50);
  handler.apply(potion, p);
  EXPECT_EQ(p.getMana(), 0);
}

TEST(PlayerTest, WarriorCannotMeditate) {
  static auto race = makeRace();
  static auto cls = makeClass();
  static auto config = makeConfig();
  Player p(1, "warrior", race, cls, 100, 0, config);

  p.startMeditating();
  EXPECT_FALSE(p.isMeditating());
}

TEST(PlayerTest, MageCanUseMagic) {
  static auto race = makeRace();
  static auto cls = makeMageClass();
  static auto config = makeConfig();
  Player p(1, "mage", race, cls, 80, 100, config);

  EXPECT_TRUE(p.spendMana(10));
  EXPECT_EQ(p.getMana(), 90);
}

// ─── GameFormulas ────────────────────────────────────────────────────────────

TEST(GameFormulasTest, MaxGoldLevel1) {
  GameFormulas f;
  EXPECT_EQ(f.calcMaxGold(1), 100u);
}

TEST(GameFormulasTest, ExpLimitLevel1) {
  GameFormulas f;
  EXPECT_EQ(f.calcExpLimit(1), 1000u);
}

TEST(GameFormulasTest, ExpLimitLevel2) {
  GameFormulas f;
  uint32_t limit = f.calcExpLimit(2);
  EXPECT_GT(limit, 3000u);
  EXPECT_LT(limit, 4000u);
}

TEST(GameFormulasTest, ExcessGoldZeroIfUnderMax) {
  GameFormulas f;
  EXPECT_EQ(f.calcExcessGold(80, 100), 0u);
}

TEST(GameFormulasTest, ExcessGoldCalculatedCorrectly) {
  GameFormulas f;
  EXPECT_EQ(f.calcExcessGold(150, 100), 50u);
}