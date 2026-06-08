
#ifndef TALLER_TP_NPCMOVEMESSAGE_H
#define TALLER_TP_NPCMOVEMESSAGE_H

#pragma once

#include "common/network/messages/message.h"
#include "common/network/protocol/serverOpCode.h"

#include <cstdint>

// Mensaje del servidor al cliente para avisar que un NPC cambió de posición.
class NpcMoveMessage : public Message {
private:

    uint32_t npcId;

    // Posición en píxeles, no en tiles.
    uint16_t x;
    uint16_t y;

public:

    NpcMoveMessage(uint32_t npcId, uint16_t x, uint16_t y);
    uint8_t opCode() const override;
    void serializeBody(PacketWriter& writer) const override;
    uint32_t getNpcId() const;
    uint16_t getX() const;
    uint16_t getY() const;
};


#endif //TALLER_TP_NPCMOVEMESSAGE_H
