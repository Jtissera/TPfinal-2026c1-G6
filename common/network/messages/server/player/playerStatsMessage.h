#pragma once
#include "../../message.h"
#include "../../../protocol/packetWriter.h"
#include "../../../protocol/serverOpCode.h"
#include <cstdint>

class PlayerStatsMessage : public Message
{
public:
    PlayerStatsMessage(uint8_t level,
                       int16_t hp, int16_t maxHp,
                       int16_t mana, int16_t maxMana,
                       uint32_t exp, uint32_t expLimit,
                       uint32_t gold);

    uint8_t getLevel() const;
    int16_t getHp() const;
    int16_t getMaxHp() const;
    int16_t getMana() const;
    int16_t getMaxMana() const;
    uint32_t getExp() const;
    uint32_t getExpLimit() const;
    uint32_t getGold() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint8_t level;
    int16_t hp;
    int16_t maxHp;
    int16_t mana;
    int16_t maxMana;
    uint32_t exp;
    uint32_t expLimit;
    uint32_t gold;
};