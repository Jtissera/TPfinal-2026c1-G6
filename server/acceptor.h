#include <cstdint>
#include <iostream>
#include <list>
#include <memory>
#include <utility>

#include <sys/socket.h>

#include "../common/liberror.h"
#include "../common/network/sockets.h"

#include "network/serverProtocolFactory.h"
#include "clientRegistry.h"
#include "receiverRegistry.h"
#include "clientHandler.h"
#include "clientMessage.h"
#include "monitorQueues.h"
#include "../common/queue.h"
#include "../common/thread.h"
#include "gameManager.h"

class Acceptor : public Thread
{
public:
    Acceptor(Socket &&acceptorSocket,
             Queue<ClientMessage> &lobbyQueue,
             Monitor &lobbyMonitor,
             ClientRegistry &clientRegistry,
             GameManager &gameManager,
             ReceiverRegistry &receiverRegistry);

    void run() override;
    void stop() override;

    Acceptor(const Acceptor &) = delete;
    Acceptor &operator=(const Acceptor &) = delete;

private:
    void reap();
    void clear();

    ServerProtocolFactory factory;
    Socket acceptorSocket;
    Queue<ClientMessage> &lobbyQueue;
    Monitor &lobbyMonitor;
    GameManager &gameManager;
    ReceiverRegistry &receiverRegistry;
    ClientRegistry &clientRegistry;
    std::list<std::unique_ptr<ClientHandler>> clients;
    uint32_t nextClientId = 1;
    std::mutex clientsMutex;
    std::atomic<bool> running{false};
};