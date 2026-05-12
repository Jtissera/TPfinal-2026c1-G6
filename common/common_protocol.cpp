#include "common_protocol.h"


void Protocol::sendUint8(uint8_t val) {
    if (skt.sendall(&val, sizeof(val)) == 0)
        throw ClosedSocket();
}

uint8_t Protocol::recvUint8() {
    uint8_t val;
    if (skt.recvall(&val, sizeof(val)) == 0)
        throw ClosedSocket();
    return val;
}



Protocol::Protocol(Socket& skt) : skt(skt) {}



void Protocol::sendPlayerName(const std::string& name) {
    uint8_t len = static_cast<uint8_t>(name.size());
    sendUint8(len);
    if (skt.sendall(name.data(), len) == 0)
        throw ClosedSocket();
}

void Protocol::sendCommand(uint8_t code) {
    sendUint8(code);
}

Message Protocol::recvMessage() {
    Message msg;
    msg.type = recvUint8();
    return msg;
}



std::string Protocol::recvPlayerName() {
    uint8_t len = recvUint8();
    std::string name(len, '\0');
    if (skt.recvall(name.data(), len) == 0)
        throw ClosedSocket();
    return name;
}

Command Protocol::recvCommand() {
    Command cmd;
    cmd.type = recvUint8();
    return cmd;
}

void Protocol::sendMessage(const Message& msg) {
    sendUint8(msg.type);
}

void Protocol::close() {
    // placeholder
}