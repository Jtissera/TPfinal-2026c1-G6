#ifndef RECEIVER_H
#define RECEIVER_H

#include <iostream>
#include <string>
#include <utility>

#include "../common/common_command.h"
#include "../common/common_liberror.h"
#include "../common/common_socket.h"
#include "../common/common_protocol.h"

#include "server_queue.h"
#include "server_thread.h"

class Receiver: public Thread {

private:
    Socket& peer;
    Queue<Command>& gameQueue;
    Protocol protocol;

public:
    Receiver(Socket& s, Queue<Command>& queue);

    void run() override;
};

#endif
