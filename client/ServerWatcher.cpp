#include "ServerWatcher.h"

#include <iostream>
#include <chrono>

#include <SDL2/SDL.h>

#include "common/network/sockets.h"
#include "common/liberror.h"

ServerWatcher::ServerWatcher(const std::string &hostname,
                             const std::string &servname,
                             std::atomic<bool> &serverShutdownDetected)
    : hostname(hostname),
      servname(servname),
      serverShutdownDetected(serverShutdownDetected)
{
}

void ServerWatcher::pause() {
    isPaused = true;
    std::unique_lock<std::mutex> lck(mtxPause);
    cvPausedAck.wait(lck, [this] {
        return isActuallyPaused.load() || !should_keep_running();
    });
}

void ServerWatcher::resume() {
    {
        std::lock_guard<std::mutex> lck(mtxPause);
        isPaused = false;
    }
    cvPause.notify_one(); 
}

void ServerWatcher::run()
{
    bool algunVezConecto = false;

    while (should_keep_running())
    {
        {
            std::unique_lock<std::mutex> lck(mtxPause);
            isActuallyPaused = true;
            cvPausedAck.notify_one();

            cvPause.wait(lck, [this] { 
                return !isPaused || !should_keep_running(); 
            });

            isActuallyPaused = false;
        }

        if (!should_keep_running()) break;

        bool conectoAhora = false;
        try
        {
            Socket probe(hostname.c_str(), servname.c_str());
            probe.close();

            conectoAhora = true;
        }
        catch (const LibError &)
        {
        }
        catch (const std::exception &e)
        {
            std::cerr << "[ServerWatcher] Excepción inesperada en probe: "
                      << e.what() << std::endl;
        }

        if (conectoAhora)
        {
            algunVezConecto = true;
        }
        else if (algunVezConecto)
        {
            std::cerr << "[ServerWatcher] Servidor no disponible. "
                         "Cerrando cliente."
                      << std::endl;
            serverShutdownDetected = true;

            SDL_Event evento;
            evento.type = SDL_QUIT;
            SDL_PushEvent(&evento);

            return;
        }

        std::unique_lock<std::mutex> lck(mtxSleep);
        cvSleep.wait_for(lck,
                         std::chrono::seconds(INTERVALO_SEG),
                         [this]
                         { return !should_keep_running(); });
    }
}

void ServerWatcher::stop()
{
    Thread::stop();

    cvSleep.notify_all();
}