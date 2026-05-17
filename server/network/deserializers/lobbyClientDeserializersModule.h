#pragma once

#include "../../../common/network/protocol/registry.h"
#include "../../../common/network/messages/client/lobby/listGamesMessage.h"
#include "../../../common/network/messages/client/lobby/createGameMessage.h"
#include "../../../common/network/messages/client/lobby/joinGameMessage.h"
#include "../../../common/network/protocol/clientOpCode.h"

class LobbyClientDeserializersModule
{
public:
    void registerDeserializers(Registry &registry) const;
};