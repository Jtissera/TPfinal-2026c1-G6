#pragma once

#include "../sockets.h"
#include "../messages/message.h"
#include <memory>
#include "registry.h"
#include "packetReader.h"
#include "packetWriter.h"
#include <arpa/inet.h>
#include <cstdint>
#include <vector>
#include "../../liberror.h"

class Protocol
{
protected:
    Socket &socket;
    std::shared_ptr<const Registry> registry;

public:
    explicit Protocol(Socket &socket, std::shared_ptr<const Registry> registry);

    void send(const Message &message);
    std::unique_ptr<Message> receive();
};