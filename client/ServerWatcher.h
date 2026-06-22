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
    void pause();
    void resume();

private:
    std::string hostname;
    std::string servname;

    std::atomic<bool> &serverShutdownDetected;

    std::mutex mtxSleep;
    std::condition_variable cvSleep;

    std::atomic<bool> isPaused{false};
    std::mutex mtxPause;
    std::condition_variable cvPause;
    std::atomic<bool> isActuallyPaused{false}; 
    std::condition_variable cvPausedAck;        

    static constexpr int INTERVALO_SEG = 3;
};