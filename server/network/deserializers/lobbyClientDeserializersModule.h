#pragma once

#include "../../../common/network/deserializerModule.h"
#include "../../../common/network/messages/client/auth/loginMessage.h"
#include "../../../common/network/messages/client/lobby/createGameMessage.h"
#include "../../../common/network/messages/client/lobby/joinGameMessage.h"
#include "../../../common/network/messages/client/lobby/leaveGameMessage.h"
#include "../../../common/network/messages/client/lobby/listGamesMessage.h"
#include "../../../common/network/protocol/clientOpCode.h"
#include "../../../common/network/protocol/registry.h"

class LobbyClientDeserializersModule : public DeserializerModule
{
public:
  void registerDeserializers(Registry &registry) const override;
};