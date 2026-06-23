#include <gtest/gtest.h>
#include "server/city/cityCommandParser.h"

class CityCommandParserTest : public ::testing::Test
{
protected:
    CityCommandParser parser;
};

TEST_F(CityCommandParserTest, ParseResucitar)
{
    auto cmd = parser.parse("/resucitar");
    ASSERT_TRUE(cmd.has_value());
    EXPECT_EQ(cmd->type, CityCommand::Type::RESURRECT);
}

TEST_F(CityCommandParserTest, ParseCurar)
{
    auto cmd = parser.parse("/curar");
    ASSERT_TRUE(cmd.has_value());
    EXPECT_EQ(cmd->type, CityCommand::Type::HEAL);
}

TEST_F(CityCommandParserTest, ParseComprar)
{
    auto cmd = parser.parse("/comprar espada");
    ASSERT_TRUE(cmd.has_value());
    EXPECT_EQ(cmd->type, CityCommand::Type::BUY);
    EXPECT_EQ(cmd->itemName, "espada");
}

TEST_F(CityCommandParserTest, ParseVender)
{
    auto cmd = parser.parse("/vender hacha");
    ASSERT_TRUE(cmd.has_value());
    EXPECT_EQ(cmd->type, CityCommand::Type::SELL);
    EXPECT_EQ(cmd->itemName, "hacha");
}

TEST_F(CityCommandParserTest, ParseDepositarItem)
{
    auto cmd = parser.parse("/depositar espada");
    ASSERT_TRUE(cmd.has_value());
    EXPECT_EQ(cmd->type, CityCommand::Type::DEPOSIT);
    EXPECT_FALSE(cmd->isGold);
    EXPECT_EQ(cmd->itemName, "espada");
}

TEST_F(CityCommandParserTest, ParseDepositarOro)
{
    auto cmd = parser.parse("/depositar oro 100");
    ASSERT_TRUE(cmd.has_value());
    EXPECT_EQ(cmd->type, CityCommand::Type::DEPOSIT);
    EXPECT_TRUE(cmd->isGold);
    EXPECT_EQ(cmd->goldAmount, 100u);
}

TEST_F(CityCommandParserTest, ParseRetirarOro)
{
    auto cmd = parser.parse("/retirar oro 250");
    ASSERT_TRUE(cmd.has_value());
    EXPECT_EQ(cmd->type, CityCommand::Type::WITHDRAW);
    EXPECT_TRUE(cmd->isGold);
    EXPECT_EQ(cmd->goldAmount, 250u);
}

TEST_F(CityCommandParserTest, ParseRetirarItem)
{
    auto cmd = parser.parse("/retirar pocion_vida");
    ASSERT_TRUE(cmd.has_value());
    EXPECT_EQ(cmd->type, CityCommand::Type::WITHDRAW);
    EXPECT_FALSE(cmd->isGold);
    EXPECT_EQ(cmd->itemName, "pocion_vida");
}

TEST_F(CityCommandParserTest, ComandoInvalidoSinSlash)
{
    EXPECT_FALSE(parser.parse("comprar espada").has_value());
}

TEST_F(CityCommandParserTest, ComandoDesconocido)
{
    EXPECT_FALSE(parser.parse("/bailar").has_value());
}

TEST_F(CityCommandParserTest, ComprarSinItem)
{
    EXPECT_FALSE(parser.parse("/comprar").has_value());
}

TEST_F(CityCommandParserTest, DepositarOroSinCantidad)
{
    EXPECT_FALSE(parser.parse("/depositar oro").has_value());
}

TEST_F(CityCommandParserTest, DepositarOroCantidadCero)
{
    EXPECT_FALSE(parser.parse("/depositar oro 0").has_value());
}