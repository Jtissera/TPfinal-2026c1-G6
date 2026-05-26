#include "server.h"

Server::Server(const char* servname)
    : classRepo(toml::parse_file("config/game.toml"))
    , raceRepo(toml::parse_file("config/game.toml"))
    , playerFactory(classRepo, raceRepo)
    , playerRepo()
    , lobbyMonitor()
    , lobbyQueue()
    , clientRegistry()
    , receiverRegistry()
    , gameManager()
    , lobbyHandler(lobbyQueue, lobbyMonitor, gameManager,
                   clientRegistry, receiverRegistry,
                   playerRepo, playerFactory)
    , socket(servname)
    , acceptor(std::move(socket), lobbyQueue, lobbyMonitor,
               clientRegistry, gameManager, receiverRegistry)
{}

int Server::run() {
    lobbyHandler.start();
    acceptor.start();

    while (std::cin.get() != 'q') {}

    acceptor.stop();
    acceptor.join();

    lobbyHandler.stop();
    lobbyHandler.join();

    gameManager.stopAll();
    return 0;
}