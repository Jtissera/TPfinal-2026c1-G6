#pragma once

#include <string>

#include "../../message.h"
#include "../../../protocol/serverOpCode.h"
#include "../../../protocol/packetWriter.h"

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