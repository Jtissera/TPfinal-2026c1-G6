#pragma once

#include "../common/network/sockets.h"
#include "../common/queue.h"
#include "clientMessage.h"
#include "monitorQueues.h"
#include "gameManager.h"
#include "lobbyHandler.h"
#include "acceptor.h"
#include "receiverRegistry.h"

class Server
{
public:
    explicit Server(const char *servname);
    int run();

private:
    Monitor lobbyMonitor;
    Queue<ClientMessage> lobbyQueue;
    ClientRegistry clientRegistry;
    ReceiverRegistry receiverRegistry;
    GameManager gameManager;
    LobbyHandler lobbyHandler;
    Socket socket;
    Acceptor acceptor;
};