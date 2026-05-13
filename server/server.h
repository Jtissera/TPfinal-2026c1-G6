#pragma once

#include <utility>

#include "../common/network/sockets.h"

#include "acceptor.h"
#include "clientMessage.h"
#include "gameLoop.h"
#include "monitorQueues.h"
#include "queue.h"

class Server
{
public:
    explicit Server(const char *servname);
    int run();

    Server(const Server &) = delete;
    Server &operator=(const Server &) = delete;

private:
    Monitor monitor;
    Queue<ClientMessage> gameQueue;
    GameLoop gameLoop;
    Socket socket;
    Acceptor acceptor;
};
