#pragma once
#include "../game/player/Player.h"
#include "../bank/bankRepository.h"
#include "cityResult.h"
#include <string>

class BankerHandler
{
public:
    explicit BankerHandler(BankRepository &bankRepo);

    CityResult handleDeposit(Player &player, const std::string &itemName);
    CityResult handleDepositGold(Player &player, uint32_t amount);
    CityResult handleWithdraw(Player &player, const std::string &itemName);
    CityResult handleWithdrawGold(Player &player, uint32_t amount);

private:
    BankRepository &bankRepo;
};