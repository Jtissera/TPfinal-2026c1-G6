#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>

#include "common/thread.h"

// ServerWatcher — hilo que monitorea si el servidor sigue activo.

class ServerWatcher : public Thread
{
public:
    ServerWatcher(const std::string &hostname, const std::string &servname,
                  std::atomic<bool> &serverShutdownDetected);

    void run() override;

    void stop() override;

private:
    std::string hostname;
    std::string servname;

    std::atomic<bool> &serverShutdownDetected;

    std::mutex mtxSleep;
    std::condition_variable cvSleep;

    static constexpr int INTERVALO_SEG = 3;
};