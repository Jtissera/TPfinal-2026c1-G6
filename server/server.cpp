#include "server.h"

Server::Server(const char *servname)
    : config(toml::parse_file("config/game.toml")), classRepo(config),
      raceRepo(config), npcRepo(config), itemRepo(config), npcFactory(npcRepo),
      playerFactory(classRepo, raceRepo, config), playerRepo(), lobbyMonitor(),
      lobbyQueue(), leaveQueue(), transitionQueue(), receiverRegistry(),
      characterArchive("data/characters.dat", "data/characters.idx"),
      playerArchive("data/players.dat", "data/players.idx", itemRepo, raceRepo,
                    classRepo, config),
      gameArchive("data/games.dat", "data/games.idx"),
      gameManager(npcFactory, itemRepo, leaveQueue, transitionQueue, config,
                  playerArchive, gameArchive),
      lobbyHandler(lobbyQueue, leaveQueue, transitionQueue, lobbyMonitor,
                   gameManager, receiverRegistry, playerRepo, playerFactory,
                   playerArchive, characterArchive, config),
      socket(servname), acceptor(std::move(socket), lobbyQueue, lobbyMonitor,
                                 gameManager, receiverRegistry) {}
int Server::run() {
  playerArchive.start(); // nuevo: arrancar el hilo de persistencia
  lobbyHandler.start();
  gameManager.restoreFromArchive();
  acceptor.start();

  while (std::cin.get() != 'q') {
  }

  acceptor.stop();
  acceptor.join();

  lobbyHandler.stop();
  lobbyHandler.join();

  gameManager.stopAll(); // adentro va a flushear todos los jugadores

  playerArchive.stop(); // cierra la queue
  playerArchive.join(); // espera que el hilo drene todo a disco

  return 0;
}