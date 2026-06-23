#include "receiverRegistry.h"

void ReceiverRegistry::add(uint32_t clientId, Receiver &receiver)
{
  std::unique_lock<std::mutex> lock(mutex);
  entries[clientId] = &receiver;
}

void ReceiverRegistry::remove(uint32_t clientId)
{
  std::unique_lock<std::mutex> lock(mutex);
  entries.erase(clientId);
}

Receiver *ReceiverRegistry::get(uint32_t clientId) const
{
  std::unique_lock<std::mutex> lock(mutex);
  std::unordered_map<uint32_t, Receiver *>::const_iterator it =
      entries.find(clientId);
  if (it == entries.end())
  {
    return nullptr;
  }
  return it->second;
}