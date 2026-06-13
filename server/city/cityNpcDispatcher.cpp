#include "cityNpcDispatcher.h"

CityNpcDispatcher::CityNpcDispatcher(PriestHandler &priest,
                                     MerchantHandler &merchant,
                                     BankerHandler &banker)
    : priest(priest), merchant(merchant), banker(banker) {}

CityResult CityNpcDispatcher::dispatch(NpcType npcType,
                                       const CityCommand &cmd,
                                       Player &player)
{
    switch (npcType)
    {
    case NpcType::PRIEST:
        return dispatchPriest(cmd, player);
    case NpcType::MERCHANT:
        return dispatchMerchant(cmd, player);
    case NpcType::BANKER:
        return dispatchBanker(cmd, player);
    default:
        return {false, "Este NPC no es de ciudad."};
    }
}

CityResult CityNpcDispatcher::dispatchPriest(const CityCommand &cmd,
                                             Player &player)
{
    switch (cmd.type)
    {
    case CityCommand::Type::RESURRECT:
        return priest.handleResurrect(player);
    case CityCommand::Type::HEAL:
        return priest.handleHeal(player);
    case CityCommand::Type::BUY:
        return priest.handleBuy(player, cmd.itemName);
    case CityCommand::Type::LIST:
        return priest.handleList();
    default:
        return {false, "El sacerdote no entiende ese comando."};
    }
}

CityResult CityNpcDispatcher::dispatchMerchant(const CityCommand &cmd,
                                               Player &player)
{
    switch (cmd.type)
    {
    case CityCommand::Type::BUY:
        return merchant.handleBuy(player, cmd.itemName);
    case CityCommand::Type::SELL:
        return merchant.handleSell(player, cmd.itemName);
    case CityCommand::Type::LIST:
        return merchant.handleList();
    default:
        return {false, "El comerciante no entiende ese comando."};
    }
}

CityResult CityNpcDispatcher::dispatchBanker(const CityCommand &cmd,
                                             Player &player)
{
    switch (cmd.type)
    {
    case CityCommand::Type::DEPOSIT:
        return cmd.isGold
                   ? banker.handleDepositGold(player, cmd.goldAmount)
                   : banker.handleDeposit(player, cmd.itemName);
    case CityCommand::Type::WITHDRAW:
        return cmd.isGold
                   ? banker.handleWithdrawGold(player, cmd.goldAmount)
                   : banker.handleWithdraw(player, cmd.itemName);
    default:
        return {false, "El banquero no entiende ese comando."};
    }
}