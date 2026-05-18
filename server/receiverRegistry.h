#pragma once

#include <cstdint>
#include <mutex>
#include <unordered_map>

class Receiver;

class ReceiverRegistry
{
public:
    ReceiverRegistry() = default;

    void add(uint32_t clientId, Receiver &receiver);
    void remove(uint32_t clientId);

    // Devuelve nullptr si el cliente no existe
    Receiver *get(uint32_t clientId) const;

    ReceiverRegistry(const ReceiverRegistry &) = delete;
    ReceiverRegistry &operator=(const ReceiverRegistry &) = delete;

private:
    mutable std::mutex mutex;
    std::unordered_map<uint32_t, Receiver *> entries;
};