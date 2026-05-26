#include "charClientDeserializersModule.h"

void CharClientDeserializersModule::registerDeserializers(Registry& registry) const {
    registry.registerDeserializer(
        static_cast<uint8_t>(ClientOpCode::MSG_CREATE_CHAR),
        [](PacketReader& reader) -> std::unique_ptr<Message> {
            auto name  = reader.readString();
            auto raza  = reader.readString();
            auto clase = reader.readString();
            return std::make_unique<CreateCharMessage>(std::move(name),
                                                       std::move(raza),
                                                       std::move(clase));
        });
}