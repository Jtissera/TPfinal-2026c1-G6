#include "serverProtocolFactory.h"

ServerProtocolFactory::ServerProtocolFactory() : registry(buildRegistry()) {}

Protocol ServerProtocolFactory::createProtocol(Socket &socket) const
{
  return Protocol(socket, registry);
}

std::shared_ptr<const Registry> ServerProtocolFactory::buildRegistry()
{
  auto registry = std::make_shared<Registry>();

  AuthClientDeserializersModule auth;
  auth.registerDeserializers(*registry);

  LobbyClientDeserializersModule lobby;
  lobby.registerDeserializers(*registry);

  GameClientDeserializersModule game;
  game.registerDeserializers(*registry);

  CharClientDeserializersModule character;
  character.registerDeserializers(*registry);

  // ACA LOS VAMOS AGREGANDO CUANDO VAMOS REALIZANDO LAS FUNCIONALIDADES

  return registry;
}
