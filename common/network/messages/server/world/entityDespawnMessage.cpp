#include "entityDespawnMessage.h"

EntityDespawnMessage::EntityDespawnMessage(uint32_t id) : id(id) {}

uint8_t EntityDespawnMessage::opCode() const {
    return static_cast<uint8_t>(ServerOpCode::MSG_ENTITY_DESPAWN);
}

void EntityDespawnMessage::serializeBody(PacketWriter& writer) const {
    writer.writeUint32(id);
}

uint32_t EntityDespawnMessage::getId() const { return id; }