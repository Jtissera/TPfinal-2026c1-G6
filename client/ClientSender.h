
#ifndef TALLER_TP_CLIENTSENDER_H
#define TALLER_TP_CLIENTSENDER_H
#include "common/queue.h"
#include "common/thread.h"
#include "common/network/protocol/protocol.h"


class ClientSender : public Thread {
public:
    ClientSender(Protocol protocol, Queue<std::shared_ptr<const Message>>& clientQueue);
    void run() override;
private:
    Protocol protocol;
    Queue<std::shared_ptr<const Message>>& clientQueue;
};







#endif //TALLER_TP_CLIENTSENDER_H
