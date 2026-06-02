
#ifndef TALLER_TP_RESURRECTMESSAGE_H
#define TALLER_TP_RESURRECTMESSAGE_H

#pragma once

#include "../../message.h"
#include "../../../protocol/clientOpCode.h"
#include "../../../protocol/packetWriter.h"

class ResurrectMessage : public Message {
public:
    ResurrectMessage() = default;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter& writer) const override;
};

#endif //TALLER_TP_RESURRECTMESSAGE_H
