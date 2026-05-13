#include "acceptor.h"

Acceptor::Acceptor(Socket &&acceptorSocket, Queue<ClientMessage> &gameQueue, Monitor &monitor)
    : factory(),
      acceptorSocket(std::move(acceptorSocket)),
      gameQueue(gameQueue),
      monitor(monitor) {}

void Acceptor::run()
{
    try
    {
        while (true)
        {
            Socket peer = acceptorSocket.accept();
            reap();

            uint32_t clientId = nextClientId++;
            auto handler = std::make_unique<ClientHandler>(std::move(peer), clientId, factory, gameQueue);

            monitor.addQueue(clientId, handler->getClientQueue());
            handler->start();
            clients.push_back(std::move(handler));
        }
    }
    catch (const LibError &)
    {
    }
    catch (const std::exception &e)
    {
        std::cerr << "[Acceptor] unexpected error: " << e.what() << std::endl;
    }
    clear();
}

void Acceptor::stop()
{
    try
    {
        acceptorSocket.shutdown(SHUT_RDWR);
        acceptorSocket.close();
    }
    catch (const LibError &)
    {
    }
    catch (const std::exception &e)
    {
        std::cerr << "[Acceptor] stop error: " << e.what() << std::endl;
    }
}

void Acceptor::reap()
{
    clients.remove_if([this](auto &handler)
                      {
        if (handler->isDead()) {
            monitor.removeQueue(handler->id());
            handler->stop();
            handler->join();
            return true;
        }
        return false; });
}

void Acceptor::clear()
{
    for (auto &handler : clients)
    {
        monitor.removeQueue(handler->id());
        handler->stop();
        handler->join();
    }
    clients.clear();
}