#include "server.h"

Server::Server(const char *servname) : monitor(),
                                       gameQueue(),
                                       gameLoop(gameQueue, monitor),
                                       socket(servname),
                                       acceptor(std::move(socket), gameQueue, monitor) {}

int Server::run()
{
    gameLoop.start();
    acceptor.start();

    while (std::cin.get() != 'q')
    {
    }

    acceptor.stop();
    acceptor.join();

    gameLoop.stop();
    gameLoop.join();

    return 0;
}
