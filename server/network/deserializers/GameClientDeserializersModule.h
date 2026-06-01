
#ifndef TALLER_TP_GAMECLIENTDESERIALIZERSMODULE_H
#define TALLER_TP_GAMECLIENTDESERIALIZERSMODULE_H
#include "common/network/protocol/registry.h"

class GameClientDeserializersModule {
public:
  void registerDeserializers(Registry &registry) const;
};
#endif
