#pragma once

#include "../../../../common/network/protocol/registry.h"

class GameServerDeserializersModule : public DeserializerModule {
public:
  void registerDeserializers(Registry &registry) const;
};