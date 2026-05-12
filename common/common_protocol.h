#ifndef COMMON_PROTOCOL_H_
#define COMMON_PROTOCOL_H_

#include <cstdint>
#include <string>

#include "common_command.h"
#include "common_message.h"
#include "common_socket.h"
#include "common_liberror.h"

//  protocolo minimo para verificar conexion y concurrencia.
class Protocol {
private:
    Socket& skt;

    void sendUint8(uint8_t val);
    uint8_t recvUint8();

public:
    explicit Protocol(Socket& skt);

    void sendPlayerName(const std::string& name);
    void sendCommand(uint8_t code);
    Message recvMessage();

    std::string recvPlayerName();
    Command recvCommand();
    void sendMessage(const Message& msg);

    void close();
};

#endif