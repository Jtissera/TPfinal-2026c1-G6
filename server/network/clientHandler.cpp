#include "clientHandler.h"

ClientHandler::ClientHandler(Socket &&socket, uint32_t clientId,
                             const ServerProtocolFactory &factory,
                             Queue<ClientMessage> &gameQueue)
    : peer(std::move(socket)), clientId(clientId), clientQueue(),
      receiver(factory.createProtocol(peer), clientId, gameQueue),
      sender(factory.createProtocol(peer), clientQueue) {}

void ClientHandler::start() {
  receiver.start();
  sender.start();
}

void ClientHandler::stop() {
  try {
    peer.shutdown(SHUT_RDWR);
  } catch (const LibError &) {
  } catch (const std::exception &e) {
    std::cerr << "[ClientHandler] shutdown error: " << e.what() << std::endl;
  }
  clientQueue.close();
}

void ClientHandler::join() {
  receiver.join();
  sender.join();
}

uint32_t ClientHandler::id() const { return clientId; }

Queue<std::shared_ptr<const Message>> &ClientHandler::getClientQueue() {
  return clientQueue;
}

bool ClientHandler::isDead() const {
  return !receiver.is_alive() || !sender.is_alive();
}

Receiver &ClientHandler::getReceiver() { return receiver; }