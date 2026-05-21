
#include "ClientSender.h"
#include <iostream>

ClientSender::ClientSender(Protocol protocol,
                           Queue<std::shared_ptr<const Message>>& clientQueue)
    : protocol(std::move(protocol)), clientQueue(clientQueue) {}

void ClientSender::run() {
    try {
        while (true) {
            auto msg = clientQueue.pop();
            protocol.send(*msg);
        }
    }
    catch (const ClosedQueue&) {}
    catch (const std::exception& e) {
        std::cerr << "[ClientSender] error: " << e.what() << std::endl;
    }
}
