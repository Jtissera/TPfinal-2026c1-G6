#include "levelUpMessage.h"



LevelUpMessage::LevelUpMessage(uint32_t playerId, uint8_t level)
    : playerId(playerId),
      newLevel(level) {
}

uint8_t LevelUpMessage::opCode() const {
    return static_cast<uint8_t>(ServerOpCode::MSG_LEVEL_UP);
}

void LevelUpMessage::serializeBody(PacketWriter& writer) const {
    writer.writeUint32(playerId);
    writer.writeUint8(newLevel);
}

uint32_t LevelUpMessage::getPlayerId() const {
    return playerId;
}

uint8_t LevelUpMessage::getLevel() const {
    return newLevel;
}