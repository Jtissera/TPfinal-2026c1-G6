#pragma once

#include "../../../../common/network/protocol/registry.h"

class LobbyServerDeserializersModule
{
public:
    void registerDeserializers(Registry &registry) const;

};