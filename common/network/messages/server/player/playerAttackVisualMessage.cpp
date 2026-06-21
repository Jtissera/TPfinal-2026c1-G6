#include "playerAttackVisualMessage.h"

PlayerAttackVisualMessage::PlayerAttackVisualMessage(
    uint32_t attackerId,
    uint32_t targetId,
    PlayerAttackVisualType visualType)
    : attackerId(attackerId),
      targetId(targetId),
      visualType(visualType)
{
}

uint8_t PlayerAttackVisualMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_PLAYER_ATTACK_VISUAL);
}

void PlayerAttackVisualMessage::serializeBody(PacketWriter& writer) const
{
    // ID del jugador que atacó.
    writer.writeUint32(attackerId);

    // ID de la entidad atacada: puede ser jugador o NPC.
    writer.writeUint32(targetId);

    // Tipo visual del ataque.
    writer.writeUint8(static_cast<uint8_t>(visualType));
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