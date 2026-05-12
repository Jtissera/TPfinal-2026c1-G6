#include "server_acceptor.h"

Acceptor::Acceptor(Socket&& s, Queue<Command>& gameQueue, Monitor& m):
        monitor(m), acceptor(std::move(s)), gameQueue(gameQueue) {}


void Acceptor::run() {
    try {
        while (true) {
            Socket peer = acceptor.accept();
            reap();
            clients.emplace_back(std::make_unique<ClientHandler>(std::move(peer), gameQueue));
            ClientHandler& c = *clients.back();
            monitor.addQueue(c.getClientQueue());
            c.start();
        }
    } catch (const LibError&) {
    } catch (const std::exception& e) {
        std::cerr << "Error inesperado en acceptor: " << e.what() << std::endl;
    }
    clear();
}

void Acceptor::reap() {
    clients.remove_if([this](auto& c) {
        if (c->isDead()) {
            monitor.removeQueue(c->getClientQueue());
            c->stop();
            c->join();
            return true;
        }
        return false;
    });
}

void Acceptor::clear() {
    for (auto& c: clients) {
        monitor.removeQueue(c->getClientQueue());
        c->stop();
        c->join();
    }
    clients.clear();
}
void Acceptor::stop() {
    try {
        acceptor.shutdown(SHUT_RDWR);
        acceptor.close();
    } catch (const LibError&) {
    } catch (const std::exception& e) {
        std::cerr << "Error en stop del acceptor: " << e.what() << std::endl;
    }
}
