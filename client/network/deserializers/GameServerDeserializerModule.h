#pragma once

#ifndef TALLER_TP_GAMESERVERDESERIALIZERMODULE_H
#define TALLER_TP_GAMESERVERDESERIALIZERMODULE_H

#include "common/network/protocol/registry.h"
#include "common/network/messages/server/player/entityMoveMessage.h"
#include "common/network/messages/server/player/playerStatsMessage.h"
#include "common/network/messages/server/player/playerDiedMessage.h"
#include "common/network/messages/server/system/mapChangedMessage.h"
#include "common/network/messages/server/error/errorMessage.h"

#include "common/network/protocol/serverOpCode.h"
#include "common/npcType.h"

class GameServerDeserializersModule
{
public:
    void registerDeserializers(Registry &registry) const;
};

#endif // TALLER_TP_GAMESERVERDESERIALIZERMODULE_H
