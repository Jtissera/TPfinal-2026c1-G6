#pragma once
#include <string>
#include "../../message.h"
#include "../../../protocol/clientOpCode.h"
#include "../../../protocol/packetWriter.h"

class CreateCharMessage : public Message {
public:
    CreateCharMessage(std::string name, std::string raza, std::string clase)
        : name(std::move(name)), raza(std::move(raza)), clase(std::move(clase)) {}

    uint8_t opCode() const override {
        return static_cast<uint8_t>(ClientOpCode::MSG_CREATE_CHAR);
    }

    void serializeBody(PacketWriter& writer) const override {
        writer.writeString(name);
        writer.writeString(raza);
        writer.writeString(clase);
    }

    const std::string& getName()  const { return name; }
    const std::string& getRaza()  const { return raza; }
    const std::string& getClase() const { return clase; }

private:
    std::string name;
    std::string raza;
    std::string clase;
};