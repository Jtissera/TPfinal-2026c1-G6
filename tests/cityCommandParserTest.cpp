#include <gtest/gtest.h>
#include "server/city/cityCommandParser.h"

TEST(CityCommandParser, ParseResucitar)
{
    auto cmd = CityCommandParser::parse("/resucitar");
    ASSERT_TRUE(cmd.has_value());
    EXPECT_EQ(cmd->type, CityCommand::Type::RESURRECT);
}

TEST(CityCommandParser, ParseCurar)
{
    auto cmd = CityCommandParser::parse("/curar");
    ASSERT_TRUE(cmd.has_value());
    EXPECT_EQ(cmd->type, CityCommand::Type::HEAL);
}

TEST(CityCommandParser, ParseComprar)
{
    auto cmd = CityCommandParser::parse("/comprar espada");
    ASSERT_TRUE(cmd.has_value());
    EXPECT_EQ(cmd->type, CityCommand::Type::BUY);
    EXPECT_EQ(cmd->itemName, "espada");
}

TEST(CityCommandParser, ParseVender)
{
    auto cmd = CityCommandParser::parse("/vender hacha");
    ASSERT_TRUE(cmd.has_value());
    EXPECT_EQ(cmd->type, CityCommand::Type::SELL);
    EXPECT_EQ(cmd->itemName, "hacha");
}

TEST(CityCommandParser, ParseDepositarItem)
{
    auto cmd = CityCommandParser::parse("/depositar espada");
    ASSERT_TRUE(cmd.has_value());
    EXPECT_EQ(cmd->type, CityCommand::Type::DEPOSIT);
    EXPECT_FALSE(cmd->isGold);
    EXPECT_EQ(cmd->itemName, "espada");
}

TEST(CityCommandParser, ParseDepositarOro)
{
    auto cmd = CityCommandParser::parse("/depositar oro 100");
    ASSERT_TRUE(cmd.has_value());
    EXPECT_EQ(cmd->type, CityCommand::Type::DEPOSIT);
    EXPECT_TRUE(cmd->isGold);
    EXPECT_EQ(cmd->goldAmount, 100u);
}

TEST(CityCommandParser, ParseRetirarOro)
{
    auto cmd = CityCommandParser::parse("/retirar oro 250");
    ASSERT_TRUE(cmd.has_value());
    EXPECT_EQ(cmd->type, CityCommand::Type::WITHDRAW);
    EXPECT_TRUE(cmd->isGold);
    EXPECT_EQ(cmd->goldAmount, 250u);
}

TEST(CityCommandParser, ParseRetirarItem)
{
    auto cmd = CityCommandParser::parse("/retirar pocion_vida");
    ASSERT_TRUE(cmd.has_value());
    EXPECT_EQ(cmd->type, CityCommand::Type::WITHDRAW);
    EXPECT_FALSE(cmd->isGold);
    EXPECT_EQ(cmd->itemName, "pocion_vida");
}

TEST(CityCommandParser, ComandoInvalidoSinSlash)
{
    EXPECT_FALSE(CityCommandParser::parse("comprar espada").has_value());
}

TEST(CityCommandParser, ComandoDesconocido)
{
    EXPECT_FALSE(CityCommandParser::parse("/bailar").has_value());
}

TEST(CityCommandParser, ComprarSinItem)
{
    EXPECT_FALSE(CityCommandParser::parse("/comprar").has_value());
}

TEST(CityCommandParser, DepositarOroSinCantidad)
{
    EXPECT_FALSE(CityCommandParser::parse("/depositar oro").has_value());
}

TEST(CityCommandParser, DepositarOroCantidadCero)
{
    EXPECT_FALSE(CityCommandParser::parse("/depositar oro 0").has_value());
}