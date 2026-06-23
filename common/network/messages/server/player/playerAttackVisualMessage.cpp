#include "playerAttackVisualMessage.h"

#include <algorithm>
#include <string>

class PacketWriter;

PlayerAttackVisualMessage::PlayerAttackVisualMessage(
    uint32_t attackerId,
    uint32_t targetId,
    PlayerAttackVisualType visualType,
    std::string effectId)
    : attackerId(attackerId),
      targetId(targetId),
      visualType(visualType),
      effectId(std::move(effectId))
{
}
uint8_t PlayerAttackVisualMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_PLAYER_ATTACK_VISUAL);
}

void PlayerAttackVisualMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint32(attackerId);
    writer.writeUint32(targetId);
    writer.writeUint8(static_cast<uint8_t>(visualType));
    writer.writeString(effectId);
}

uint32_t PlayerAttackVisualMessage::getAttackerId() const
{
    return attackerId;
}

uint32_t PlayerAttackVisualMessage::getTargetId() const
{
    return targetId;
}

PlayerAttackVisualType PlayerAttackVisualMessage::getVisualType() const
{
    return visualType;
}

const std::string &PlayerAttackVisualMessage::getEffectId() const
{
    return effectId;
}