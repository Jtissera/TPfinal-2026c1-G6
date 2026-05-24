#pragma once

#include <string>
#include <cstdint>

#include "../../message.h"
#include "../../../protocol/clientOpCode.h"
#include "../../../protocol/packetWriter.h"
#include "../../../../dtos/gameTypes.h"

class CreateCharMessage : public Message {
public:
    CreateCharMessage(std::string name, Raza raza, Clase clase)
        : name(std::move(name)), raza(raza), clase(clase) {}

    uint8_t opCode() const override {
        return static_cast<uint8_t>(ClientOpCode::MSG_CREATE_CHAR);
    }

    void serializeBody(PacketWriter& writer) const override {
        writer.writeString(name);
        writer.writeUint8(static_cast<uint8_t>(raza));
        writer.writeUint8(static_cast<uint8_t>(clase));
    }

    const std::string& getName() const { return name; }
    Raza  getRaza()  const { return raza; }
    Clase getClase() const { return clase; }

private:
    std::string name;
    Raza        raza;
    Clase       clase;
};
