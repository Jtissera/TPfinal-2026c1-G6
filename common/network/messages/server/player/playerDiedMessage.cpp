#include "playerDiedMessage.h"

PlayerDiedMessage::PlayerDiedMessage(uint32_t playerId)
    : playerId(playerId)
{
}

uint32_t PlayerDiedMessage::getId() const
{
    return playerId;
}

uint8_t PlayerDiedMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_PLAYER_DIED);
}

void PlayerDiedMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint32(playerId);
}