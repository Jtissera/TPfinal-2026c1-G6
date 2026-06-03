
#ifndef TALLER_TP_UNEQUIPSLOTMESSAGE_H
#define TALLER_TP_UNEQUIPSLOTMESSAGE_H

#pragma once

#include <cstdint>

#include "common/network/messages/message.h"
#include "common/network/protocol/clientOpCode.h"
#include "common/network/protocol/packetWriter.h"
#include "server/game/EquipSlot.h"

class UnequipSlotMessage : public Message {
private:
    // Slot lógico que el cliente quiere desequipar.
    // Ejemplo: HAND, ARMOR, HELMET, SHIELD.
    EquipSlot slot;

public:
    // Construye el mensaje indicando qué slot se quiere desequipar.
    explicit UnequipSlotMessage(EquipSlot slot);

    // Devuelve el opcode que identifica este mensaje.
    uint8_t opCode() const override;

    // Serializa el cuerpo del mensaje.
    // Enviamos el slot como uint8_t.
    void serializeBody(PacketWriter& writer) const override;

    // Getter usado por el server después de deserializar.
    EquipSlot getSlot() const;
};

#endif //TALLER_TP_UNEQUIPSLOTMESSAGE_H
