#include "bankerHandler.h"

BankerHandler::BankerHandler(BankRepository &bankRepo)
    : bankRepo(bankRepo) {}

CityResult BankerHandler::handleDeposit(Player &player,
                                        const std::string &itemName)
{
    auto removed = player.getInventory().removeItemByName(itemName);
    if (!removed)
        return {false, "No tenés '" + itemName + "' en el inventario."};

    bankRepo.get(player.getId()).depositItem(std::move(*removed));
    return {true, "Depositaste " + itemName + " en el banco."};
}

CityResult BankerHandler::handleDepositGold(Player &player, uint32_t amount)
{
    if (player.getGold() < amount)
        return {false, "No tenés suficiente oro."};
    player.spendGold(amount);
    bankRepo.get(player.getId()).depositGold(amount);
    return {true, "Depositaste " + std::to_string(amount) + " oro en el banco."};
}

CityResult BankerHandler::handleWithdraw(Player &player,
                                         const std::string &itemName)
{
    auto item = bankRepo.get(player.getId()).withdrawItem(itemName);
    if (!item)
        return {false, "No tenés '" + itemName + "' en el banco."};
    if (!player.getInventory().addItem(std::move(*item)))
        return {false, "Inventario lleno."};
    return {true, "Retiraste " + itemName + " del banco."};
}

CityResult BankerHandler::handleWithdrawGold(Player &player, uint32_t amount)
{
    uint32_t taken = bankRepo.get(player.getId()).withdrawGold(amount);
    if (taken == 0)
        return {false, "No tenés oro en el banco."};
    player.addGold(taken);
    return {true, "Retiraste " + std::to_string(taken) + " oro del banco."};
}