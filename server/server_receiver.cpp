#include "server_receiver.h"

#include "../common/common_command.h"

Receiver::Receiver(Socket& s, Queue<Command>& queue): peer(s), gameQueue(queue), protocol(peer) {}

void Receiver::run() {
    try {
        std::string playerName = protocol.recvPlayerName();
        std::cout << "[Receiver] Jugador conectado: " << playerName << std::endl;
        
        while (true) {
            Command cmd = protocol.recvCommand();
            cmd.playerName = playerName;
            gameQueue.push(std::move(cmd));
        }
    } catch (const ClosedSocket&) {
    } catch (const LibError&) {

    } catch (const std::exception& e) {
        std::cerr << "Error en receiver: " << e.what() << std::endl;
    }
}
