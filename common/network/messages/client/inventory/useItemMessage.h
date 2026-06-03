
#ifndef TALLER_TP_USEITEMMESSAGE_H
#define TALLER_TP_USEITEMMESSAGE_H

#pragma once

#include <cstdint>

#include "common/network/messages/message.h"
#include "common/network/protocol/clientOpCode.h"
#include "common/network/protocol/packetWriter.h"

class UseItemMessage : public Message {
private:
    // Instancia real del item en el inventario del server.
    uint32_t itemInstanceId;

public:
    // Construye el mensaje con el item concreto que se quiere usar.
    explicit UseItemMessage(uint32_t itemInstanceId);

    // Opcode del mensaje.
    uint8_t opCode() const override;

    // Serializa el instanceId.
    void serializeBody(PacketWriter& writer) const override;

    // Getter para que el server sepa qué item usar.
    uint32_t getItemInstanceId() const;
};

#endif //TALLER_TP_USEITEMMESSAGE_H
