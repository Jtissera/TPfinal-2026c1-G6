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

    if (socket.sendall(packet.data(), packet.size()) == 0)
        throw ClosedSocket();
}

std::unique_ptr<Message> Protocol::receive()
{

    uint8_t msgType;
    uint16_t bodyLengthNet;

    if (socket.recvall(&msgType, sizeof(msgType)) == 0)
        throw ClosedSocket();

    if (socket.recvall(&bodyLengthNet, sizeof(bodyLengthNet)) == 0)
        throw ClosedSocket();

    const uint16_t bodyLength = ntohs(bodyLengthNet);

    std::vector<uint8_t> body(bodyLength);
    if (bodyLength > 0)
    {
        if (socket.recvall(body.data(), bodyLength) == 0)
            throw ClosedSocket();
    }

    PacketReader reader(body.data(), body.size());
    return registry->deserialize(msgType, reader);
}