#pragma once

#include <memory>

#include "../../common/network/protocol/protocol.h"
#include "../../common/network/protocol/registry.h"
#include "deserializers/authServerDeserializersModule.h"

class Socket;

class ClientProtocolFactory
{
public:
    ClientProtocolFactory();
    Protocol createProtocol(Socket &socket) const;

private:
    std::shared_ptr<const Registry> registry;
    std::shared_ptr<const Registry> buildRegistry();
};
