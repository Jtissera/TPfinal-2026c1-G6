#include "server_sender.h"

Sender::Sender(Socket& s, Queue<Message>& queue): peer(s), protocol(peer), clientQueue(queue) {}

void Sender::run() {
    try {
        while (true) {
            Message msg = clientQueue.pop();
            protocol.sendMessage(msg);
        }
    } catch (const ClosedQueue&) {

    } catch (const LibError&) {

    } catch (const std::exception& e) {
        std::cerr << "Error en sender: " << e.what() << std::endl;
    }
}
