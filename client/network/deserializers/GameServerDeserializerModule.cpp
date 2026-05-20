//
// Created by mauro on 20/5/26.
//

#include "GameServerDeserializerModule.h"

void GameServerDeserializersModule::registerDeserializers(Registry& registry) const {
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_ENTITY_MOVE),
        [](PacketReader& reader) -> std::unique_ptr<Message> {
            auto id = reader.readUint8();
            auto x  = reader.readUint16();
            auto y  = reader.readUint16();
            return std::make_unique<EntityMoveMessage>(id, x, y);
        });
}