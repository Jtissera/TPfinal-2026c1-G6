#pragma once

#include <cstdint>
#include <iostream>
#include <utility>

#include <sys/socket.h>

#include "../../common/liberror.h"
#include "../../common/network/messages/message.h"
#include "../../common/network/sockets.h"

#include "../../common/thread.h"
#include "../clientMessage.h"
#include "../common/queue.h"
#include "protocol/serverProtocolFactory.h"
#include "receiver.h"
#include "sender.h"

class ClientHandler {
public:
  ClientHandler(Socket &&socket, uint32_t clientId,
                const ServerProtocolFactory &factory,
                Queue<ClientMessage> &gameQueue);

  void start();
  void stop();
  void join();

  uint32_t id() const;
  Queue<std::shared_ptr<const Message>> &getClientQueue();
  bool isDead() const;

  Receiver &getReceiver();

  ClientHandler(const ClientHandler &) = delete;
  ClientHandler &operator=(const ClientHandler &) = delete;

private:
  Socket peer;
  uint32_t clientId;
  Queue<std::shared_ptr<const Message>> clientQueue;
  Receiver receiver;
  Sender sender;
};