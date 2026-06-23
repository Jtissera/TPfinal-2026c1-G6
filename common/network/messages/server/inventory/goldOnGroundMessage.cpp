#include "goldOnGroundMessage.h"

GoldOnGroundMessage::GoldOnGroundMessage(uint32_t instanceId, uint32_t amount, int x, int y)
    : instanceId(instanceId),
      amount(amount),
      x(x),
      y(y)
{
}

uint32_t GoldOnGroundMessage::getInstanceId() const { return instanceId; }
uint32_t GoldOnGroundMessage::getAmount() const { return amount; }
int GoldOnGroundMessage::getX() const { return x; }
int GoldOnGroundMessage::getY() const { return y; }

uint8_t GoldOnGroundMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_GOLD_ON_GROUND);
}

void GoldOnGroundMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint32(instanceId);
    writer.writeUint32(amount);
    writer.writeUint16(static_cast<uint16_t>(x));
    writer.writeUint16(static_cast<uint16_t>(y));
}