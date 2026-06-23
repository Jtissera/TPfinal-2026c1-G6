#include "interactNpcMessage.h"

InteractNpcMessage::InteractNpcMessage(uint32_t npcId, std::string cmd)
    : npcId(npcId),
      cmd(std::move(cmd))
{
}

uint32_t InteractNpcMessage::getNpcId() const { return npcId; }
const std::string &InteractNpcMessage::getCmd() const { return cmd; }

uint8_t InteractNpcMessage::opCode() const
{
    return static_cast<uint8_t>(ClientOpCode::MSG_INTERACT_NPC);
}

void InteractNpcMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint32(npcId);
    writer.writeString(cmd);
}