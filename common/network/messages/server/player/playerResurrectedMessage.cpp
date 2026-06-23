#include "playerResurrectedMessage.h"

PlayerResurrectedMessage::PlayerResurrectedMessage(uint32_t playerId,
                                                   uint16_t tileX,
                                                   uint16_t tileY)
    : playerId(playerId),
      tileX(tileX),
      tileY(tileY)
{
}

uint32_t PlayerResurrectedMessage::getPlayerId() const { return playerId; }
uint16_t PlayerResurrectedMessage::getTileX() const { return tileX; }
uint16_t PlayerResurrectedMessage::getTileY() const { return tileY; }

uint8_t PlayerResurrectedMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_PLAYER_RESURRECTED);
}

void PlayerResurrectedMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint32(playerId);
    writer.writeUint16(tileX);
    writer.writeUint16(tileY);
}