#include "server_clientHandler.h"

ClientHandler::ClientHandler(Socket&& skt, Queue<Command>& gameQueue):
        peer(std::move(skt)),
        clientQueue(50),
        receiver(peer, gameQueue),
        sender(peer, clientQueue) {}

void ClientHandler::start() {
    receiver.start();
    sender.start();
}

void ClientHandler::stop() {
    try {
        peer.shutdown(SHUT_RDWR);

    } catch (const LibError& e) {

    } catch (const std::exception& e) {
        std::cerr << "Error en shutdown: " << e.what() << std::endl;
    }
    clientQueue.close();
}

void ClientHandler::join() {
    receiver.join();
    sender.join();
}

Queue<Message>& ClientHandler::getClientQueue() { return clientQueue; }


bool ClientHandler::isDead() { return !receiver.is_alive() || !sender.is_alive(); }
