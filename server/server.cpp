#include "server.h"

Server::Server(const char* servname)
    : classRepo(toml::parse_file("config/game.toml"))
    , raceRepo(toml::parse_file("config/game.toml"))
    , playerFactory(classRepo, raceRepo)
    , lobbyMonitor()
    , lobbyQueue()
    , charMonitor()
    , charQueue()
    , clientRegistry()
    , receiverRegistry()
    , gameManager()
    , lobbyHandler(lobbyQueue, lobbyMonitor, gameManager, clientRegistry, receiverRegistry)
    , charHandler(charQueue, charMonitor, playerRepo, playerFactory, receiverRegistry, clientRegistry, lobbyQueue)
    , socket(servname)
    , acceptor(std::move(socket), charQueue, charMonitor, clientRegistry, gameManager, receiverRegistry) {}

int Server::run() {
    lobbyHandler.start();
    charHandler.start();
    acceptor.start();

    while (std::cin.get() != 'q') {}

    acceptor.stop();
    acceptor.join();

    charHandler.stop();
    charHandler.join();

    lobbyHandler.stop();
    lobbyHandler.join();

    gameManager.stopAll();
    return 0;
}