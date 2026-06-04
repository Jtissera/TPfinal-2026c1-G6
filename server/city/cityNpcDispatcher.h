#pragma once
#include "priestHandler.h"
#include "merchantHandler.h"
#include "bankerHandler.h"
#include "cityCommandParser.h"
#include "common/npcType.h"
#include "../game/player/Player.h"

class CityNpcDispatcher
{
public:
    CityNpcDispatcher(PriestHandler &priest,
                      MerchantHandler &merchant,
                      BankerHandler &banker);

    CityResult dispatch(NpcType npcType,
                        const CityCommand &cmd,
                        Player &player);

private:
    PriestHandler &priest;
    MerchantHandler &merchant;
    BankerHandler &banker;

    CityResult dispatchPriest(const CityCommand &cmd, Player &player);
    CityResult dispatchMerchant(const CityCommand &cmd, Player &player);
    CityResult dispatchBanker(const CityCommand &cmd, Player &player);
};