#include "server_monitorQueues.h"

Monitor::Monitor() {}

void Monitor::addQueue(Queue<Message>& q) {
    std::unique_lock<std::mutex> lock(m);
    queues.push_back(std::ref(q));
}

void Monitor::removeQueue(Queue<Message>& q) {
    std::unique_lock<std::mutex> lock(m);
    queues.remove_if(
            [&q](const std::reference_wrapper<Queue<Message>>& rw) { return &rw.get() == &q; });
}

void Monitor::broadcast(const Message& msg) {
    std::unique_lock<std::mutex> lock(m);
    for (Queue<Message>& q: queues) {
        q.try_push(msg);
    }
}
