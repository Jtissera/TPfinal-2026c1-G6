//
// Created by mauro on 20/5/26.
//

#ifndef TALLER_TP_GAMECLIENTDESERIALIZERSMODULE_H
#define TALLER_TP_GAMECLIENTDESERIALIZERSMODULE_H
#include "common/network/protocol/registry.h"
#include "../../../common/network/messages/client/combat/enemyHitPlayerMessage.h"
#include "../../../common/network/messages/client/combat/resurrectMessage.h"

class GameClientDeserializersModule {
public:
    void registerDeserializers(Registry& registry) const;
};
#endif //TALLER_TP_GAMECLIENTDESERIALIZERSMODULE_H
