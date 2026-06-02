#include "entitySpawnMessage.h"

EntitySpawnMessage::EntitySpawnMessage(uint32_t id, NpcType type,
                                       uint16_t x, uint16_t y)
    : id(id), type(type), x(x), y(y) {}

uint8_t EntitySpawnMessage::opCode() const {
    return static_cast<uint8_t>(ServerOpCode::MSG_ENTITY_SPAWN);
}

void EntitySpawnMessage::serializeBody(PacketWriter& writer) const {
    writer.writeUint32(id);
    writer.writeUint8(static_cast<uint8_t>(type));
    writer.writeUint32(x);
    writer.writeUint32(y);
}

uint32_t EntitySpawnMessage::getId()   const { return id; }
NpcType  EntitySpawnMessage::getType() const { return type; }
uint16_t EntitySpawnMessage::getX()    const { return x; }
uint16_t EntitySpawnMessage::getY()    const { return y; }