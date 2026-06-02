#pragma once
#include <cstdint>
#include "../../message.h"
#include "../../../protocol/packetWriter.h"
#include "../../../protocol/serverOpCode.h"

class EntityDespawnMessage : public Message {
public:
    explicit EntityDespawnMessage(uint32_t id);

    uint8_t  opCode() const override;
    void     serializeBody(PacketWriter& writer) const override;

    uint32_t getId() const;

private:
    uint32_t id;
};