#include "authServerDeserializersModule.h"

void AuthServerDeserializersModule::registerDeserializers(Registry &registry) const
{
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_CONNECT_OK),
        [](PacketReader &) -> std::unique_ptr<Message>
        {
            return std::make_unique<ConnectOkMessage>();
        });
}