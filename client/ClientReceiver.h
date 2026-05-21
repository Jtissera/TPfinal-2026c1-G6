
#ifndef TALLER_TP_CLIENTRECEIVER_H
#define TALLER_TP_CLIENTRECEIVER_H
#include <memory>

#include "common/queue.h"
#include "common/thread.h"
#include "common/network/messages/message.h"
#include "common/network/protocol/protocol.h"

class ClientReceiver : public Thread {
public:
    ClientReceiver(Protocol protocol, Queue<std::shared_ptr<const Message>>& clientQueue);
    void run() override;
private:
    Protocol protocol;
    Queue<std::shared_ptr<const Message>>& clientQueue;
};

#endif //TALLER_TP_CLIENTRECEIVER_H
