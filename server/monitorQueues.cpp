#include "monitorQueues.h"

void Monitor::addQueue(uint32_t clientId,
                       Queue<std::shared_ptr<const Message>> &queue) {
  std::unique_lock<std::mutex> lock(mutex);
  entries.push_back(Entry{clientId, queue});
}

void Monitor::removeQueue(uint32_t clientId) {
  std::unique_lock<std::mutex> lock(mutex);
  entries.remove_if(
      [clientId](const Entry &e) { return e.clientId == clientId; });
}

Queue<std::shared_ptr<const Message>> *
Monitor::getQueue(uint32_t clientId) const {
  std::unique_lock<std::mutex> lock(mutex);
  for (const auto &entry : entries) {
    if (entry.clientId == clientId) {
      return &entry.queue;
    }
  }
  return nullptr;
}

void Monitor::sendTo(uint32_t clientId,
                     const std::shared_ptr<const Message> &message) {
  std::unique_lock<std::mutex> lock(mutex);
  for (auto &entry : entries) {
    if (entry.clientId == clientId) {
      entry.queue.try_push(message);
      return;
    }
  }
}

uint8_t Monitor::size() const {
  std::unique_lock<std::mutex> lock(mutex);
  return static_cast<uint8_t>(entries.size());
}

void Monitor::broadcast(const std::shared_ptr<const Message> &message) {
  std::unique_lock<std::mutex> lock(mutex);
  for (auto &entry : entries) {
    entry.queue.try_push(message);
  }
}
