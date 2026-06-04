#pragma once

#include <cstdint>
#include <list>
#include <memory>
#include <mutex>

#include "../common/network/messages/message.h"

#include "../common/queue.h"

class Monitor {
public:
  Monitor() = default;

  void addQueue(uint32_t clientId,
                Queue<std::shared_ptr<const Message>> &queue);

  void removeQueue(uint32_t clientId);

  Queue<std::shared_ptr<const Message>> *getQueue(uint32_t clientId) const;

  void sendTo(uint32_t clientId, const std::shared_ptr<const Message> &message);

  void broadcast(const std::shared_ptr<const Message> &message);
  void broadcastExcept(uint32_t excludeId, const std::shared_ptr<const Message> &message);

  uint8_t size() const;

  Monitor(const Monitor &) = delete;
  Monitor &operator=(const Monitor &) = delete;

private:
  struct Entry {
    uint32_t clientId;
    Queue<std::shared_ptr<const Message>> &queue;
  };

  mutable std::mutex mutex;
  std::list<Entry> entries;
};