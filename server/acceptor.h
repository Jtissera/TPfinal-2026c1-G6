#include <cstdint>
#include <iostream>
#include <list>
#include <memory>
#include <utility>

#include <sys/socket.h>

#include "../common/liberror.h"
#include "../common/network/sockets.h"

#include "network/serverProtocolFactory.h"
#include "clientHandler.h"
#include "clientMessage.h"
#include "monitorQueues.h"
#include "queue.h"
#include "thread.h"

class Acceptor : public Thread
{
public:
    Acceptor(Socket &&acceptorSocket, Queue<ClientMessage> &gameQueue, Monitor &monitor);

    void run() override;
    void stop() override;

    Acceptor(const Acceptor &) = delete;
    Acceptor &operator=(const Acceptor &) = delete;

private:
    void reap();
    void clear();

    ServerProtocolFactory factory;
    Socket acceptorSocket;
    Queue<ClientMessage> &gameQueue;
    Monitor &monitor;
    std::list<std::unique_ptr<ClientHandler>> clients;
    uint32_t nextClientId = 1;
};