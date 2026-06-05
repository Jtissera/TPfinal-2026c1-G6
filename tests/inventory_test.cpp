#include "../server/game/combat/itemEffectHandler.h"
#include "helpers/testHelpers.h"
#include <gtest/gtest.h>

// ─── Inventario ──────────────────────────────────────────────────────────────

TEST(InventoryTest, AddItemSucceeds) {
  Inventory inv = makeInventory();
  EXPECT_TRUE(inv.addItem(makeWeapon(5, 10)));
  EXPECT_EQ(inv.getItems().size(), 1u);
}

TEST(InventoryTest, EquipWeaponSucceeds) {
  Inventory inv = makeInventory();
  inv.addItem(makeWeapon(5, 10));
  EXPECT_TRUE(inv.equipItem(1));
  EXPECT_NE(inv.getEquipped(EquipSlot::HAND), nullptr);
}

TEST(InventoryTest, RemoveItemClearsEquip) {
  Inventory inv = makeInventory();
  inv.addItem(makeWeapon(5, 10));
  inv.equipItem(1);
  inv.removeItem(1);
  EXPECT_EQ(inv.getEquipped(EquipSlot::HAND), nullptr);
  EXPECT_TRUE(inv.getItems().empty());
}

TEST(InventoryTest, MaxItemsRespected) {
  Inventory inv = makeInventory();
  constexpr std::size_t MAX = 20;
  for (std::size_t i = 0; i < MAX; i++) {
    Item item = makeWeapon(1, 2);
    item.instanceId = i + 1;
    EXPECT_TRUE(inv.addItem(item));
  }
  Item extra = makeWeapon(1, 2);
  extra.instanceId = 999;
  EXPECT_FALSE(inv.addItem(extra));
}

TEST(InventoryTest, RemoveAllItemsClearsInventory) {
  Inventory inv = makeInventory();
  inv.addItem(makeWeapon(5, 10));
  inv.addItem(makeArmor(3, 5));
  auto removed = inv.removeAllItems();
  EXPECT_EQ(removed.size(), 2u);
  EXPECT_TRUE(inv.getItems().empty());
}

TEST(InventoryTest, StaffReplacesWeaponInHand) {
  Inventory inv = makeInventory();
  Item weapon = makeWeapon(5, 10);
  weapon.instanceId = 1;
  Item staff = makeStaff(2, ItemEffect::DAMAGE, 2, 4, 0, 5);

  inv.addItem(weapon);
  inv.addItem(staff);

  EXPECT_TRUE(inv.equipItem(1));
  EXPECT_TRUE(inv.equipItem(2));
  EXPECT_EQ(inv.getEquipped(EquipSlot::HAND)->instanceId, 2u);
}

TEST(InventoryTest, StaffReplacesStaff) {
  Inventory inv = makeInventory();
  Item staff1 = makeStaff(1, ItemEffect::DAMAGE, 2, 4, 0, 5);
  Item staff2 = makeStaff(2, ItemEffect::HEAL, 0, 0, 100, 100);

  inv.addItem(staff1);
  inv.addItem(staff2);

  EXPECT_TRUE(inv.equipItem(1));
  EXPECT_TRUE(inv.equipItem(2));
  EXPECT_EQ(inv.getEquipped(EquipSlot::HAND)->instanceId, 2u);
}

// ─── Efectos de items ────────────────────────────────────────────────────────

TEST(ItemEffectTest, HealthPotionHealsPlayer) {
  ItemEffectHandler handler;
  Player p = makePlayer(1, 0, 0, 50);
  Item potion = makePotion(1, 40, 0);

  EXPECT_TRUE(handler.apply(potion, p));
  EXPECT_EQ(p.getHp(), 90);
}

TEST(ItemEffectTest, HealthPotionDoesNotExceedMaxHp) {
  ItemEffectHandler handler;
  Player p = makePlayer(1, 0, 0, 100);
  Item potion = makePotion(1, 50, 0);

  handler.apply(potion, p);
  EXPECT_EQ(p.getHp(), 100);
}

TEST(ItemEffectTest, ManaPotionRestoresMana) {
  ItemEffectHandler handler;
  Player p = makeMagePlayer(1, 0, 0);
  p.spendMana(50);

  Item potion = makePotion(1, 0, 30);
  handler.apply(potion, p);
  EXPECT_GT(p.getMana(), 50);
}

TEST(ItemEffectTest, FlautaElficaHealsPlayer) {
  ItemEffectHandler handler;
  Player mage = makeMagePlayer(1, 0, 0);
  mage.takeDamage(40);

  int16_t hpBefore = mage.getHp();
  Item flauta = makeStaff(1, ItemEffect::HEAL, 0, 0, 100, 100);

  EXPECT_TRUE(handler.apply(flauta, mage));
  EXPECT_GT(mage.getHp(), hpBefore);
}

TEST(ItemEffectTest, FlautaElficaFailsWithoutMana) {
  ItemEffectHandler handler;
  Player mage = makeMagePlayer(1, 0, 0);
  mage.takeDamage(40);
  mage.spendMana(100);

  int16_t hpBefore = mage.getHp();
  Item flauta = makeStaff(1, ItemEffect::HEAL, 0, 0, 100, 100);

  EXPECT_FALSE(handler.apply(flauta, mage));
  EXPECT_EQ(mage.getHp(), hpBefore);
}