#include "npcListMessage.h"

NpcListMessage::NpcListMessage(std::vector<NpcSnapshot> npcs)
    : npcs(std::move(npcs)) {}

uint8_t NpcListMessage::opCode() const {
    return static_cast<uint8_t>(ServerOpCode::MSG_NPC_LIST);
}

void NpcListMessage::serializeBody(PacketWriter& writer) const {
    writer.writeUint16(static_cast<uint16_t>(npcs.size()));
    for (const auto& npc : npcs) {
        writer.writeUint32(npc.id);
        writer.writeUint8(static_cast<uint8_t>(npc.type));
        writer.writeUint16(npc.x);
        writer.writeUint16(npc.y);
    }
}

const std::vector<NpcSnapshot>& NpcListMessage::getNpcs() const {
    return npcs;
}