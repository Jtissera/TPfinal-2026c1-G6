#pragma once

#include "../../../../common/network/protocol/registry.h"

class ErrorDeserializersModule
{
public:
    void registerDeserializers(Registry &registry) const;
};