#ifndef SERVER_GAMELOOP_H_
#define SERVER_GAMELOOP_H_

#include <iostream>

#include "../common/common_command.h"
#include "../common/common_message.h"
#include "server_queue.h"
#include "server_thread.h"
#include "server_monitorQueues.h"

//game loop minimo que solo hace echo del comando.
class GameLoop : public Thread {
private:
    Queue<Command>& gameQueue;
    Monitor& monitor;

public:
    GameLoop(Queue<Command>& q, Monitor& m);

    void run() override;
    void stop() override;
};

#endif