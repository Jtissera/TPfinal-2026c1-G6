#include "errorDeserializersModule.h"

#include "../../../../common/network/messages/server/error/errorMessage.h"
#include "../../../../common/network/protocol/serverOpCode.h"

void ErrorDeserializersModule::registerDeserializers(Registry &registry) const
{
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_ERROR),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            auto reason = reader.readString();
            return std::make_unique<ErrorMessage>(std::move(reason));
        });
}