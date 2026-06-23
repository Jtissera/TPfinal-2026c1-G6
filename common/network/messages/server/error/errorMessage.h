#pragma once
#include "../../message.h"
#include "../../../protocol/packetWriter.h"
#include "../../../protocol/serverOpCode.h"
#include <cstdint>
#include <string>

class ErrorMessage : public Message
{
public:
    explicit ErrorMessage(std::string reason);

    const std::string &getReason() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    std::string reason;
};