
#pragma once

#include "common/network/messages/message.h"
#include "common/network/protocol/serverOpCode.h"
#include <cstdint>

class LevelUpMessage : public Message {
private:
    // ID del jugador que subió de nivel.
    const uint32_t playerId;

    // Nuevo nivel del jugador.
    const uint8_t newLevel;

public:
    // Constructor usado por el server.
    LevelUpMessage(uint32_t playerId, uint8_t level);

    // Opcode del mensaje.
    uint8_t opCode() const override;

    // Serializa: primero playerId, después level.
    void serializeBody(PacketWriter& writer) const override;

    // ID del jugador que subió de nivel.
    uint32_t getPlayerId() const;

    // Nuevo nivel.
    uint8_t getLevel() const;
};