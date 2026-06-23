#include "serverProtocolFactory.h"

ServerProtocolFactory::ServerProtocolFactory()
    : registry(buildRegistry()) {}

Protocol ServerProtocolFactory::createProtocol(Socket &socket) const
{
  return Protocol(socket, registry);
}

std::shared_ptr<const Registry> ServerProtocolFactory::buildRegistry()
{
  std::shared_ptr<Registry> reg = std::make_shared<Registry>();

  AuthClientDeserializersModule auth;
  LobbyClientDeserializersModule lobby;
  GameClientDeserializersModule game;
  CharClientDeserializersModule character;

  auth.registerDeserializers(*reg);
  lobby.registerDeserializers(*reg);
  game.registerDeserializers(*reg);
  character.registerDeserializers(*reg);

  return reg;
}