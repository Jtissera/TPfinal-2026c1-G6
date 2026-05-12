#include "protocol.h"

Protocol::Protocol(Socket &socket, std::shared_ptr<const Registry> registry)
    : socket(socket), registry(std::move(registry)) {}

void Protocol::send(const Message &message)
{
    PacketWriter bodyWriter;
    message.serializeBody(bodyWriter);

    PacketWriter packet;
    packet.writeUint8(message.opCode());
    packet.writeUint16(static_cast<uint16_t>(bodyWriter.size()));
    packet.writeBytes(bodyWriter.data(), bodyWriter.size());

    socket.sendall(packet.data(), packet.size());
}

std::unique_ptr<Message> Protocol::receive()
{
    uint8_t opcode;
    uint16_t bodyLengthNet;

    socket.recvall(&opcode, sizeof(opcode));
    socket.recvall(&bodyLengthNet, sizeof(bodyLengthNet));

    const uint16_t bodyLength = ntohs(bodyLengthNet);

    std::vector<uint8_t> body(bodyLength);

    if (bodyLength > 0)
        socket.recvall(body.data(), bodyLength);

    PacketReader reader(body.data(), body.size());
    return registry->deserialize(opcode, reader);
}