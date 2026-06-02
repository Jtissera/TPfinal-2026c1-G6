#pragma once
#include <cstdint>
#include "../../message.h"
#include "../../../protocol/packetWriter.h"
#include "../../../protocol/serverOpCode.h"
#include "../../../../../common/npcType.h"

class EntitySpawnMessage : public Message {
public:
    EntitySpawnMessage(uint32_t id, NpcType type, uint16_t x, uint16_t y);

    uint8_t  opCode() const override;
    void     serializeBody(PacketWriter& writer) const override;

    uint32_t getId()   const;
    NpcType  getType() const;
    uint16_t getX()    const;
    uint16_t getY()    const;

private:
    uint32_t id;
    NpcType  type;
    uint16_t x;
    uint16_t y;
};