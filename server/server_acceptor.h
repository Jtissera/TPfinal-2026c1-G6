#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include <iostream>
#include <list>
#include <memory>
#include <utility>

#include <sys/socket.h>

#include "../common/common_command.h"
#include "../common/common_liberror.h"
#include "../common/common_socket.h"

#include "server_clientHandler.h"
#include "server_monitorQueues.h"
#include "server_queue.h"
#include "server_thread.h"

class Acceptor: public Thread {

private:
    Monitor& monitor;
    Socket acceptor;
    Queue<Command>& gameQueue;
    std::list<std::unique_ptr<ClientHandler>> clients;

public:
    Acceptor(Socket&& s, Queue<Command>& gameQueue, Monitor& m);

    void run() override;
    void stop() override;

    void reap();
    void clear();

    Acceptor(const Acceptor&) = delete;
    Acceptor& operator=(const Acceptor&) = delete;
};

#endif
