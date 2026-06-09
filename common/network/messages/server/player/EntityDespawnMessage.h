#ifndef TALLER_TP_ENTITYDESPAWNMESSAGE_H
#define TALLER_TP_ENTITYDESPAWNMESSAGE_H

#include <cstdint>
#include "../../message.h"
#include "../../../protocol/packetWriter.h"
#include "../../../protocol/serverOpCode.h"

class EntityDespawnMessage : public Message {
public:
    explicit EntityDespawnMessage(uint32_t id);

    uint8_t opCode() const override;
    void serializeBody(PacketWriter& writer) const override;

    uint32_t getEntityId() const;

private:
    uint32_t entityId;
};

#endif