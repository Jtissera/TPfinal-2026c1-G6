#pragma once
#include "../../message.h"
#include "../../../protocol/packetWriter.h"
#include "../../../protocol/serverOpCode.h"
#include "../../../../../common/npcType.h"
#include <cstdint>
#include <vector>

struct NpcSnapshot
{
    uint32_t id;
    NpcType type;
    uint16_t x;
    uint16_t y;
};

class NpcListMessage : public Message
{
public:
    explicit NpcListMessage(std::vector<NpcSnapshot> npcs);

    const std::vector<NpcSnapshot> &getNpcs() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    std::vector<NpcSnapshot> npcs;
};