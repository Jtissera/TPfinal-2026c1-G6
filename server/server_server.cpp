#include "server_server.h"

Server::Server(const char* servname):
        monitor(),
        gameQueue(),
        gameLoop(gameQueue, monitor),
        socket(servname),
        acceptor(std::move(socket), gameQueue, monitor) {}

int Server::run() {
    gameLoop.start();
    acceptor.start();

    while (std::cin.get() != 'q') {}

    acceptor.stop();  // detiene nuevas conexiones y limpia clientes
    acceptor.join();

    gameLoop.stop();  // cierra la queue y el loop
    gameLoop.join();

    return 0;
}
