#ifndef MONITOR_H
#define MONITOR_H


#include <list>

#include "mutex"
#include "server_clientHandler.h"


class Monitor {
    std::mutex m;
    std::list<std::reference_wrapper<Queue<Message>>> queues;

public:
    Monitor();
    void addQueue(Queue<Message>& q);
    void removeQueue(Queue<Message>& q);
    void broadcast(const Message& msg);
};

#endif
