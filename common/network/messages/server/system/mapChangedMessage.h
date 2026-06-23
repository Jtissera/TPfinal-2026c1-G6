#pragma once
#include "../../message.h"
#include "../../../protocol/packetWriter.h"
#include "../../../protocol/serverOpCode.h"
#include <cstdint>
#include <string>

class MapChangedMessage : public Message
{
public:
    explicit MapChangedMessage(std::string mapPath);

    const std::string &getMapPath() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    std::string mapPath;
};