#include "playerStatsMessage.h"

PlayerStatsMessage::PlayerStatsMessage(uint8_t level,
                                       int16_t hp,    int16_t maxHp,
                                       int16_t mana,  int16_t maxMana,
                                       uint32_t exp,  uint32_t expLimit,
                                       uint32_t gold)
    : level(level)
    , hp(hp),       maxHp(maxHp)
    , mana(mana),   maxMana(maxMana)
    , exp(exp),     expLimit(expLimit)
    , gold(gold)
{}

uint8_t PlayerStatsMessage::opCode() const {
    return static_cast<uint8_t>(ServerOpCode::MSG_PLAYER_STATS);
}

void PlayerStatsMessage::serializeBody(PacketWriter& writer) const {
    writer.writeUint8(level);
    writer.writeUint16(hp);
    writer.writeUint16(maxHp);
    writer.writeUint16(mana);
    writer.writeUint16(maxMana);
    writer.writeUint32(exp);
    writer.writeUint32(expLimit);
    writer.writeUint32(gold);
}