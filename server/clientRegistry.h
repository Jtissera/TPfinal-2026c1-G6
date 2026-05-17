#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "../common/network/messages/message.h"
#include "../common/queue.h"

class ClientRegistry
{
public:
    void add(uint32_t clientId, Queue<std::shared_ptr<const Message>> &clientQueue);
    void remove(uint32_t clientId);

    // Devuelve nullptr si el cliente no existe
    Queue<std::shared_ptr<const Message>> *get(uint32_t clientId) const;

    ClientRegistry(const ClientRegistry &) = delete;
    ClientRegistry &operator=(const ClientRegistry &) = delete;

    ClientRegistry() = default;

private:
    mutable std::mutex mutex;
    std::unordered_map<uint32_t, Queue<std::shared_ptr<const Message>> *> entries;
};