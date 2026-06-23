#pragma once
#include "../../message.h"
#include "../../../protocol/clientOpCode.h"
#include "../../../protocol/packetWriter.h"
#include <cstdint>
#include <string>

class ConnectMessage : public Message
{
public:
    ConnectMessage(uint8_t protocolVersion, std::string username);

    uint8_t version() const;
    const std::string &getUsername() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint8_t protocolVersion;
    std::string username;
};