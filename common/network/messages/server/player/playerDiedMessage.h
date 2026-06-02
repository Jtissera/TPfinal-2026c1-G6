// PlayerDiedMessage.h
// PlayerDiedMessage.h
#pragma once

#include <cstdint>

#include "common/network/messages/message.h"

class PlayerDiedMessage : public Message {
private:
    // Id del jugador que murió.
    uint32_t id;

public:
    // Constructor: recibe el id del jugador muerto.
    explicit PlayerDiedMessage(uint32_t id);

    // Devuelve el opcode correspondiente a MSG_PLAYER_DIED.
    uint8_t opCode() const override;

    // Serializa el cuerpo del mensaje.
    void serializeBody(PacketWriter& writer) const override;

    // Permite consultar el id del jugador muerto.
    uint32_t getId() const;
};