#include "acceptor.h"

Acceptor::Acceptor(Socket &&acceptorSocket, Queue<ClientMessage> &lobbyQueue,
                   Monitor &lobbyMonitor, GameManager &gameManager,
                   ReceiverRegistry &receiverRegistry)
    : factory(), acceptorSocket(std::move(acceptorSocket)),
      lobbyQueue(lobbyQueue), lobbyMonitor(lobbyMonitor),
      gameManager(gameManager), receiverRegistry(receiverRegistry) {}

void Acceptor::run() {
  running = true;
  std::thread reaper([this]() {
    while (running) {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      std::unique_lock<std::mutex> lock(clientsMutex);
      reap();
    }
  });

  try {
    while (true) {
      Socket peer = acceptorSocket.accept();

      uint32_t clientId = nextClientId++;

      std::cout << "[Acceptor] Nueva conexión aceptada. ID asignado: " << clientId << std::endl;
      auto handler = std::make_unique<ClientHandler>(std::move(peer), clientId,
                                                     factory, lobbyQueue);

      lobbyMonitor.addQueue(clientId, handler->getClientQueue());
      receiverRegistry.add(clientId, handler->getReceiver());
      handler->start();

      {
        std::unique_lock<std::mutex> lock(clientsMutex);
        clients.push_back(std::move(handler));
      }
    }
  } catch (const LibError &) {
  } catch (const std::exception &e) {
    std::cerr << "[Acceptor] unexpected error: " << e.what() << std::endl;
  }

  running = false;
  reaper.join();
  clear();
}

void Acceptor::stop() {
  running = false;
  try {
    acceptorSocket.shutdown(SHUT_RDWR);
    acceptorSocket.close();
  } catch (const LibError &) {
  } catch (const std::exception &e) {
    std::cerr << "[Acceptor] stop error: " << e.what() << std::endl;
  }
}

void Acceptor::reap() {
  // ya se llama con clientsMutex tomado
  clients.remove_if([this](auto &handler) {
    if (handler->isDead()) {
      lobbyMonitor.removeQueue(handler->id());
      gameManager.removeClient(handler->id());
      receiverRegistry.remove(handler->id());
      handler->stop();
      handler->join();
      return true;
    }
    return false;
  });
}

void Acceptor::clear() {
  std::unique_lock<std::mutex> lock(clientsMutex);
  for (auto &handler : clients) {
    lobbyMonitor.removeQueue(handler->id());
    gameManager.removeClient(handler->id());
    receiverRegistry.remove(handler->id());
    handler->stop();
    handler->join();
  }
  clients.clear();
}