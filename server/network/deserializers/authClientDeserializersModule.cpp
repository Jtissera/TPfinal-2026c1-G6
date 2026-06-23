#include "authClientDeserializersModule.h"

void AuthClientDeserializersModule::registerDeserializers(
    Registry &registry) const
{
    registry.registerDeserializer(
        static_cast<uint8_t>(ClientOpCode::MSG_CONNECT),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            uint8_t version = reader.readUint8();
            std::string username = reader.readString();
            return std::make_unique<ConnectMessage>(version, std::move(username));
        });
}