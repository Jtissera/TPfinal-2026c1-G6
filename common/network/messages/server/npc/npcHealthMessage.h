
#ifndef TALLER_TP_NPCHEALTHMESSAGE_H
#define TALLER_TP_NPCHEALTHMESSAGE_H

#pragma once
#include "../../message.h"
#include "../../../protocol/packetWriter.h"
#include "../../../protocol/serverOpCode.h"
#include <cstdint>

class NpcHealthMessage : public Message {
public:
    NpcHealthMessage(
        uint32_t npcId,
        int16_t hp,
        int16_t maxHp
    );

    uint8_t opCode() const override;

    void serializeBody(PacketWriter& writer) const override;

private:
    uint32_t npcId;
    int16_t hp;
    int16_t maxHp;
};

#endif //TALLER_TP_NPCHEALTHMESSAGE_H

