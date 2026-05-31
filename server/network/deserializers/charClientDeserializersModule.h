#pragma once
#include "../../../common/network/messages/client/auth/createCharMessage.h"
#include "../../../common/network/protocol/registry.h"
#include "../../../common/network/protocol/clientOpCode.h"
#include "../../../common/network/deserializerModule.h"

class CharClientDeserializersModule : public DeserializerModule {
public:
    void registerDeserializers(Registry& registry) const override;
};