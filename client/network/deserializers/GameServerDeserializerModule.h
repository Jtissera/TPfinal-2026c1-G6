
#ifndef TALLER_TP_GAMESERVERDESERIALIZERMODULE_H
#define TALLER_TP_GAMESERVERDESERIALIZERMODULE_H


#pragma once
#include "../../../common/network/protocol/registry.h"
#include "../../../common/network/messages/server/player/EntityMoveMessage.h"
#include "../../../common/network/protocol/serverOpCode.h"

class GameServerDeserializersModule {
public:
    void registerDeserializers(Registry& registry) const;
};
#endif //TALLER_TP_GAMESERVERDESERIALIZERMODULE_H
