#include <gtest/gtest.h>
#include "server/game/combatSystem.h" // O el path donde esté tu CombatSystem
#include "server/game/Player.h"       // Asegurate de incluir lo necesario
#include "server/game/gameFormulas.h"

TEST(CombatSystem, NewbieNoPuedeAtacarHastaNivel13) {
    GameFormulas formulas;
    
    static RaceStats race = {
        .name="humano", .health=1, .mana=1, .recovery=1,
        .constitution=10, .intelligence=10, .strength=10, .agility=1
    };
    static ClassStats cls = {
        .name="guerrero", .health=1, .mana=0, .meditation=0, .canUseMagic=false
    };

    Player attacker(1, "test", race, cls, 10000, 0);
    Player target  (2, "test", race, cls, 10000, 0);
    attacker.setPos(0, 0);
    target.setPos  (0, 0);

    CombatSystem combat;

    // Subir de nivel hasta 13 dando exp manualmente
    while (attacker.getLevel() < 13) {
        uint32_t limit = formulas.calcExpLimit(attacker.getLevel());
        attacker.addExperience(limit);
    }
    while (target.getLevel() < 13) {
        uint32_t limit = formulas.calcExpLimit(target.getLevel());
        target.addExperience(limit);
    }

    // En nivel 12 todavía son newbies
    // En nivel 13 ya pueden atacar
    EXPECT_EQ(attacker.getLevel(), 13);
    auto result = combat.attack(attacker, target);
    EXPECT_TRUE(result.valid);
}

TEST(Player, LevelUpAlAcumularExp) {
    static RaceStats race = {
        .name="humano", .health=1, .mana=1, .recovery=1,
        .constitution=10, .intelligence=10, .strength=10, .agility=1
    };
    static ClassStats cls = {
        .name="guerrero", .health=1, .mana=0, .meditation=0, .canUseMagic=false
    };

    GameFormulas formulas;
    Player p(1, "test", race, cls, 10000, 0);
    EXPECT_EQ(p.getLevel(), 1);

    uint32_t limit = formulas.calcExpLimit(1);
    p.addExperience(limit);

    EXPECT_EQ(p.getLevel(), 2);
}

TEST(Player, ExpSobranteNoSePierde) {
    static RaceStats race = {
        .name="humano", .health=1, .mana=1, .recovery=1,
        .constitution=10, .intelligence=10, .strength=10, .agility=1
    };
    static ClassStats cls = {
        .name="guerrero", .health=1, .mana=0, .meditation=0, .canUseMagic=false
    };

    GameFormulas formulas;
    Player p(1, "test", race, cls, 10000, 0);

    uint32_t limit = formulas.calcExpLimit(1);
    p.addExperience(limit + 50);

    EXPECT_EQ(p.getLevel(), 2);
    EXPECT_EQ(p.getExp(), 50u);
}