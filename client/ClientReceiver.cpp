

#include "ClientReceiver.h"
#include <iostream>

ClientReceiver::ClientReceiver(Protocol protocol,
                               Queue<std::shared_ptr<const Message>>& clientQueue)
    : protocol(std::move(protocol)), clientQueue(clientQueue) {}

void ClientReceiver::run() {
    try {
        while (true) {
            // unique_ptr del protocol.receive()
            auto msg = protocol.receive();

            // lo movemos a shared_ptr para la cola
            clientQueue.push(std::shared_ptr<const Message>(std::move(msg)));
        }
    }
    catch (const ClosedSocket&) {}
    catch (const std::exception& e) {
        std::cerr << "[ClientReceiver] error: " << e.what() << std::endl;
    }
}