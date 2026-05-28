// tests/inventoryTest.cpp
#include <gtest/gtest.h>
#include "server/game/inventory.h"

Item makeWeapon(uint32_t id) {
    Item i;
    i.id     = id;
    i.slot   = ItemSlot::WEAPON;
    i.effect = ItemEffect::DAMAGE;
    i.stats.damageMin = 2;
    i.stats.damageMax = 5;
    return i;
}

TEST(Inventory, AddItemHastaMaximo) {
    Inventory inv;
    for (size_t i = 1; i <= Inventory::MAX_ITEMS; i++)
        EXPECT_TRUE(inv.addItem(makeWeapon(i)));
    EXPECT_FALSE(inv.addItem(makeWeapon(999))); 
}

TEST(Inventory, EquipYGetEquipped) {
    Inventory inv;
    inv.addItem(makeWeapon(1));
    EXPECT_TRUE(inv.equipItem(1));
    EXPECT_NE(inv.getEquipped(EquipSlot::HAND), nullptr);
}

TEST(Inventory, RemoveDesequipa) {
    Inventory inv;
    inv.addItem(makeWeapon(1));
    inv.equipItem(1);
    inv.removeItem(1);
    EXPECT_EQ(inv.getEquipped(EquipSlot::HAND), nullptr);
}

TEST(Inventory, RemoveItemInexistente) {
    Inventory inv;
    auto result = inv.removeItem(999);
    EXPECT_FALSE(result.has_value());
}

TEST(Inventory, NoEquiparArmaYBaculoALaVez) {
    Inventory inv;
    Item arma  = makeWeapon(1);
    Item baculo;
    baculo.id   = 2;
    baculo.slot = ItemSlot::STAFF;
    inv.addItem(arma);
    inv.addItem(baculo);
    inv.equipItem(1);
    inv.equipItem(2); // debería pisar la mano
    // ambos van a HAND, el segundo pisa al primero
    EXPECT_EQ(inv.getEquipped(EquipSlot::HAND)->id, 2u);
}

TEST(Inventory, PotionNoEquipable) {
    Inventory inv;
    Item potion;
    potion.id   = 1;
    potion.slot = ItemSlot::CONSUMABLE;
    inv.addItem(potion);
    EXPECT_FALSE(inv.equipItem(1));
}

TEST(Inventory, GetItemsDevuelveTodos) {
    Inventory inv;
    inv.addItem(makeWeapon(1));
    inv.addItem(makeWeapon(2));
    EXPECT_EQ(inv.getItems().size(), 2u);
}

TEST(Inventory, RemoveReduceLista) {
    Inventory inv;
    inv.addItem(makeWeapon(1));
    inv.addItem(makeWeapon(2));
    inv.removeItem(1);
    EXPECT_EQ(inv.getItems().size(), 1u);
}

TEST(Inventory, UnequipSlot) {
    Inventory inv;
    inv.addItem(makeWeapon(1));
    inv.equipItem(1);
    EXPECT_TRUE(inv.unequipSlot(EquipSlot::HAND));
    EXPECT_EQ(inv.getEquipped(EquipSlot::HAND), nullptr);
}

TEST(Inventory, UnequipSlotVacio) {
    Inventory inv;
    EXPECT_FALSE(inv.unequipSlot(EquipSlot::HAND));
}