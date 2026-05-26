#pragma once

#include "../../../../common/network/protocol/registry.h"
#include "../../../common/network/deserializerModule.h"
#include "../../../../common/network/protocol/serverOpCode.h"
#include "../../../../common/network/protocol/packetReader.h"
#include "../../../common/network/messages/server/auth/connectOKMessage.h"
#include "../../../common/network/messages/server/auth/createOkMessage.h"

class AuthServerDeserializersModule : public DeserializerModule
{
public:
    void registerDeserializers(Registry &registry) const override;
};