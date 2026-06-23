#include "playerHealthMessage.h"

PlayerHealthMessage::PlayerHealthMessage(uint32_t playerId, uint16_t hp, uint16_t hpMax)
    : playerId(playerId),
      hp(hp),
      hpMax(hpMax)
{
}

uint32_t PlayerHealthMessage::getPlayerId() const { return playerId; }
uint16_t PlayerHealthMessage::getHp() const { return hp; }
uint16_t PlayerHealthMessage::getHpMax() const { return hpMax; }

uint8_t PlayerHealthMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_PLAYER_HEALTH);
}

void PlayerHealthMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint32(playerId);
    writer.writeUint16(hp);
    writer.writeUint16(hpMax);
}