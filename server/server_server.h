#ifndef SERVER_H
#define SERVER_H

#include <utility>

#include "../common/common_socket.h"

#include "server_acceptor.h"
#include "server_gameLoop.h"
#include "server_monitorQueues.h"
#include "server_queue.h"

class Server {
    Monitor monitor;
    Queue<Command> gameQueue;
    GameLoop gameLoop;
    Socket socket;
    Acceptor acceptor;

public:
    explicit Server(const char* servname);
    int run();
};

#endif
