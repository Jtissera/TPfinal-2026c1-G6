#include "protocol/registry.h"

#pragma once

class DeserializerModule
{
public:
    virtual ~DeserializerModule() = default;

    virtual void registerDeserializers(Registry &registry) const = 0;
};