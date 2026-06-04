#include "server.h"

Server::Server(const char *servname)
    : config(toml::parse_file("config/game.toml")), classRepo(config),
      raceRepo(config), npcRepo(config), itemRepo(config), npcFactory(npcRepo),
      playerFactory(classRepo, raceRepo, config), playerRepo(), lobbyMonitor(),
      lobbyQueue(), leaveQueue(), transitionQueue(), receiverRegistry(),
      gameManager(npcFactory, itemRepo, leaveQueue, transitionQueue, config),
      lobbyHandler(lobbyQueue, leaveQueue, transitionQueue, lobbyMonitor, gameManager,
                   receiverRegistry, playerRepo, playerFactory),
      socket(servname), acceptor(std::move(socket), lobbyQueue, lobbyMonitor,
                                 gameManager, receiverRegistry) {}

int Server::run()
{
  lobbyHandler.start();
  acceptor.start();

  while (std::cin.get() != 'q')
  {
  }

  acceptor.stop();
  acceptor.join();

  lobbyHandler.stop();
  lobbyHandler.join();

  gameManager.stopAll();
  return 0;
}