#ifndef HANDLER_H
#define HANDLER_H

#include <utility>

#include <sys/socket.h>

#include "../common/common_command.h"
#include "../common/common_message.h"
#include "../common/common_socket.h"

#include "server_queue.h"
#include "server_receiver.h"
#include "server_sender.h"
#include "server_thread.h"

class ClientHandler {

private:
    Socket peer;
    Queue<Message> clientQueue;
    Receiver receiver;
    Sender sender;

public:
    ClientHandler(Socket&& peer, Queue<Command>& gameQueue);

    void start();
    void stop();
    void join();
    void pushClient(const Message& msg);
    bool isDead();
    Queue<Message>& getClientQueue();
};

#endif
