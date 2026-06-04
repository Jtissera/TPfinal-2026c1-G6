#include <gtest/gtest.h>
#include "server/resurrection/resurrectionSystem.h"

TEST(ResurrectionSystem, EncolaYTickea)
{
    ResurrectionSystem sys;
    sys.enqueue(1, 5, 5, 1000.0f);
    EXPECT_TRUE(sys.isPending(1));
}

TEST(ResurrectionSystem, NoCompletaAntesDelTiempo)
{
    ResurrectionSystem sys;
    sys.enqueue(1, 5, 5, 1000.0f);
    int called = 0;
    sys.tick(500.0f, [&](uint32_t, int, int)
             { called++; });
    EXPECT_EQ(called, 0);
    EXPECT_TRUE(sys.isPending(1));
}

TEST(ResurrectionSystem, CompletaAlLlegarAcero)
{
    ResurrectionSystem sys;
    sys.enqueue(1, 5, 5, 1000.0f);
    int tx = 0, ty = 0;
    uint32_t resId = 0;
    sys.tick(1000.0f, [&](uint32_t id, int x, int y)
             {
        resId = id; tx = x; ty = y; });
    EXPECT_EQ(resId, 1u);
    EXPECT_EQ(tx, 5);
    EXPECT_EQ(ty, 5);
    EXPECT_FALSE(sys.isPending(1));
}

TEST(ResurrectionSystem, CompletaConDeltaMayor)
{
    ResurrectionSystem sys;
    sys.enqueue(1, 3, 4, 500.0f);
    bool called = false;
    sys.tick(600.0f, [&](uint32_t, int, int)
             { called = true; });
    EXPECT_TRUE(called);
}

TEST(ResurrectionSystem, CancelElimina)
{
    ResurrectionSystem sys;
    sys.enqueue(1, 5, 5, 1000.0f);
    sys.cancel(1);
    EXPECT_FALSE(sys.isPending(1));
}

TEST(ResurrectionSystem, MultiplesJugadores)
{
    ResurrectionSystem sys;
    sys.enqueue(1, 1, 1, 500.0f);
    sys.enqueue(2, 2, 2, 1500.0f);
    int completed = 0;
    sys.tick(600.0f, [&](uint32_t, int, int)
             { completed++; });
    EXPECT_EQ(completed, 1); // solo jugador 1
    EXPECT_FALSE(sys.isPending(1));
    EXPECT_TRUE(sys.isPending(2));
}