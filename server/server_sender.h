#ifndef SENDER_H
#define SENDER_H

#include <iostream>

#include "../common/common_liberror.h"
#include "../common/common_message.h"
#include "../common/common_socket.h"
#include "../common/common_socket.h"
#include "../common/common_protocol.h"

#include "server_queue.h"
#include "server_thread.h"

class Sender: public Thread {

private:
    Socket& peer;
    Protocol protocol;
    Queue<Message>& clientQueue;

public:
    Sender(Socket& s, Queue<Message>& queue);

    void run() override;
};

#endif
