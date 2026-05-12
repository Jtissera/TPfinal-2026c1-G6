#pragma once

#include <cstdint>
#include <string>

#include "../../message.h"
#include "../../../protocol/clientOpCode.h"
#include "../../../protocol/packetWriter.h"

class ConnectMessage : public Message
{

public:
    explicit ConnectMessage(uint8_t protocolVersion, std::string username);

    uint8_t version() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;
    const std::string &getUsername() const;

private:
    uint8_t protocolVersion;
    std::string username;
};