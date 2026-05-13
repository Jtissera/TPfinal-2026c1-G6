#pragma once

#include <iostream>
#include <memory>

#include "../common/network/protocol/protocol.h"
#include "../common/liberror.h"
#include "../common/network/sockets.h"

#include "queue.h"
#include "thread.h"

class Sender : public Thread
{
public:
    Sender(Protocol protocol, Queue<std::shared_ptr<const Message>> &clientQueue);

    void run() override;

private:
    Protocol protocol;
    Queue<std::shared_ptr<const Message>> &clientQueue;
};
