
#include "npcHealthMessage.h"


NpcHealthMessage::NpcHealthMessage(
    uint32_t npcId,
    int16_t hp,
    int16_t maxHp
)
    : npcId(npcId)
    , hp(hp)
    , maxHp(maxHp)
{}

uint8_t NpcHealthMessage::opCode() const {
    return static_cast<uint8_t>(ServerOpCode::MSG_NPC_HEALTH);
}

void NpcHealthMessage::serializeBody(PacketWriter& writer) const {
    writer.writeUint32(npcId);
    writer.writeUint16(static_cast<uint16_t>(hp));
    writer.writeUint16(static_cast<uint16_t>(maxHp));
}

uint32_t NpcHealthMessage::getNpcId() const {
    return npcId;
}

uint16_t NpcHealthMessage::getHp() const {
    return hp;
}

uint16_t NpcHealthMessage::getMaxHp() const {
    return maxHp;
}
