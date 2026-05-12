#include "clientProtocolFactory.h"

ClientProtocolFactory::ClientProtocolFactory() : registry(buildRegistry()) {}

Protocol ClientProtocolFactory::createProtocol(Socket &socket) const
{
    return Protocol(socket, registry);
}

std::shared_ptr<const Registry> ClientProtocolFactory::buildRegistry()
{
    auto registry = std::make_shared<Registry>();

    AuthServerDeserializersModule auth;
    auth.registerDeserializers(*registry);
    // ACA LOS VAMOS AGREGANDO CUANDO VAMOS REALIZANDO LAS FUNCIONALIDADES

    return registry;
}