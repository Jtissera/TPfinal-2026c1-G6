#pragma once

#include <memory>

#include "../../common/network/protocol/protocol.h"
#include "../../common/network/protocol/registry.h"
#include "deserializers/authClientDeserializersModule.h"

class Socket;

class ServerProtocolFactory
{
public:
    ServerProtocolFactory();
    Protocol createProtocol(Socket &socket) const;

private:
    std::shared_ptr<const Registry> registry;
    std::shared_ptr<const Registry> buildRegistry();
};
