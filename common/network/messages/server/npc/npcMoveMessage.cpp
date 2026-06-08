
#include "npcMoveMessage.h"
#include "common/network/messages/server/npc/npcMoveMessage.h"

NpcMoveMessage::NpcMoveMessage(uint32_t npcId, uint16_t x, uint16_t y)
    : npcId(npcId),
      x(x),
      y(y) {
}

uint8_t NpcMoveMessage::opCode() const {
    return static_cast<uint8_t>(ServerOpCode::MSG_NPC_MOVE);
}

void NpcMoveMessage::serializeBody(PacketWriter& writer) const {
    writer.writeUint32(npcId);
    writer.writeUint16(x);
    writer.writeUint16(y);
}

uint32_t NpcMoveMessage::getNpcId() const {
    return npcId;
}

uint16_t NpcMoveMessage::getX() const {
    return x;
}

uint16_t NpcMoveMessage::getY() const {
    return y;
}

