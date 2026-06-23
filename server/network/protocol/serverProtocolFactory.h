#pragma once

#include <memory>

#include "../../../common/network/protocol/protocol.h"
#include "../../../common/network/protocol/registry.h"
#include "../deserializers/GameClientDeserializersModule.h"
#include "../deserializers/authClientDeserializersModule.h"
#include "../deserializers/charClientDeserializersModule.h"
#include "../deserializers/lobbyClientDeserializersModule.h"

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
