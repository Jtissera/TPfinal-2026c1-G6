#include "joinOkMessage.h"

JoinOkMessage::JoinOkMessage(uint32_t gameId, std::string gameName,
                             uint16_t spawnX, uint16_t spawnY)
    : gameId(gameId), gameName(std::move(gameName))
    , spawnX(spawnX), spawnY(spawnY) {}

uint32_t JoinOkMessage::getGameId() const { return gameId; }
const std::string& JoinOkMessage::getGameName() const { return gameName; }
uint16_t JoinOkMessage::getSpawnX() const { return spawnX; }
uint16_t JoinOkMessage::getSpawnY() const { return spawnY; }

uint8_t JoinOkMessage::opCode() const {
    return static_cast<uint8_t>(ServerOpCode::MSG_JOIN_OK);
}

void JoinOkMessage::serializeBody(PacketWriter& writer) const {
    writer.writeUint32(gameId);
    writer.writeString(gameName);
    writer.writeUint16(spawnX);
    writer.writeUint16(spawnY);
}