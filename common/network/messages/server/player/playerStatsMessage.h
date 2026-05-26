#pragma once

#include "../../message.h"
#include "../../../protocol/packetWriter.h"
#include "../../../protocol/serverOpCode.h"
#include <cstdint>

class PlayerStatsMessage : public Message {
public:
    PlayerStatsMessage(uint8_t level,
                       int16_t hp,    int16_t maxHp,
                       int16_t mana,  int16_t maxMana,
                       uint32_t exp,  uint32_t expLimit,
                       uint32_t gold);

    uint8_t opCode() const override;
    void serializeBody(PacketWriter& writer) const override;

    uint8_t getLevel() const { return level; }

    int16_t getHp() const { return hp; }
    int16_t getMaxHp() const { return maxHp; }

    int16_t getMana() const { return mana; }
    int16_t getMaxMana() const { return maxMana; }

    uint32_t getExp() const { return exp; }
    uint32_t getExpLimit() const { return expLimit; }

    uint32_t getGold() const { return gold; }

private:
    uint8_t  level;
    int16_t  hp,    maxHp;
    int16_t  mana,  maxMana;
    uint32_t exp,   expLimit;
    uint32_t gold;
};