#include "clientRegistry.h"

void ClientRegistry::add(uint32_t clientId, Queue<std::shared_ptr<const Message>> &clientQueue)
{
    std::unique_lock<std::mutex> lock(mutex);
    entries[clientId] = &clientQueue;
}

void ClientRegistry::remove(uint32_t clientId)
{
    std::unique_lock<std::mutex> lock(mutex);
    entries.erase(clientId);
}

Queue<std::shared_ptr<const Message>> *ClientRegistry::get(uint32_t clientId) const
{
    std::unique_lock<std::mutex> lock(mutex);
    auto it = entries.find(clientId);
    if (it == entries.end())
        return nullptr;
    return it->second;
}