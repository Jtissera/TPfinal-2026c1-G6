#include "entityDespawnMessage.h"

EntityDespawnMessage::EntityDespawnMessage(uint32_t entityId)
    : entityId(entityId)
{
}

uint32_t EntityDespawnMessage::getEntityId() const
{
    return entityId;
}

uint8_t EntityDespawnMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_ENTITY_DESPAWN);
}

void EntityDespawnMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint32(entityId);
}