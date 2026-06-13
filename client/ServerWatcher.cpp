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

void ServerWatcher::run()
{
    bool algunVezConecto = false;

    while (should_keep_running())
    {
        bool conectoAhora = false;
        try
        {
            Socket probe(hostname.c_str(), servname.c_str());

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