#include "client_client.h"

Client::Client(const char* hostname, const char* servname)
    : skt(hostname, servname), protocol(skt) {}

int Client::run() {
    try {
        std::cout << "Nombre de jugador: ";
        std::string name;
        std::getline(std::cin, name);

        protocol.sendPlayerName(name);

        std::cout << "Conectado. Escribi un numero (0-255) para enviar comando, "
                     "o 'exit' para salir."
                  << std::endl;

        std::string line;

        while (std::getline(std::cin, line)) {
            if (line == "exit")
                break;

            try {
                uint8_t code = static_cast<uint8_t>(std::stoi(line));

                protocol.sendCommand(code);

                Message msg = protocol.recvMessage();
                std::cout << "[Server] echo type=" << (int)msg.type << std::endl;

            } catch (const std::invalid_argument&) {
                std::cerr << "Comando invalido, ingresa un numero." << std::endl;
            }
        }

        protocol.close();

    } catch (const ClosedSocket&) {
        std::cerr << "Conexion cerrada por el servidor." << std::endl;
        return 1;

    } catch (const LibError& e) {
        std::cerr << "Error de conexion: " << e.what() << std::endl;
        return 1;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}