#include "pickItemMessage.h"

PickItemMessage::PickItemMessage(uint32_t instanceId, bool isGold)
    : instanceId(instanceId),
      isGold(isGold)
{
}

uint32_t PickItemMessage::getInstanceId() const { return instanceId; }
bool PickItemMessage::getIsGold() const { return isGold; }

uint8_t PickItemMessage::opCode() const
{
    return static_cast<uint8_t>(ClientOpCode::MSG_PICK_ITEM);
}

void PickItemMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint32(instanceId);
    writer.writeUint8(isGold ? 1 : 0);
}