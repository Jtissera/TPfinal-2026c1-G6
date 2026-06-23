#pragma once
#include "../../message.h"
#include "../../../protocol/packetWriter.h"
#include "../../../protocol/serverOpCode.h"
#include <cstdint>

class EntityDespawnMessage : public Message
{
public:
    explicit EntityDespawnMessage(uint32_t entityId);

    uint32_t getEntityId() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint32_t entityId;
};