#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/packetWriter.h"
#include "common/network/protocol/serverOpCode.h"
#include "common/npcType.h"
#include <cstdint>
#include <string>

class NpcSpawnMessage : public Message
{
public:
    NpcSpawnMessage(uint32_t npcId,
                    NpcType type,
                    std::string name,
                    uint16_t x,
                    uint16_t y,
                    uint16_t hp,
                    uint16_t hpMax,
                    uint16_t level,
                    bool hostile);

    uint32_t getNpcId() const;
    NpcType getType() const;
    const std::string &getName() const;
    uint16_t getX() const;
    uint16_t getY() const;
    uint16_t getHp() const;
    uint16_t getHpMax() const;
    uint16_t getLevel() const;
    bool isHostile() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint32_t npcId;
    NpcType type;
    std::string name;
    uint16_t x;
    uint16_t y;
    uint16_t hp;
    uint16_t hpMax;
    uint16_t level;
    bool hostile;
};