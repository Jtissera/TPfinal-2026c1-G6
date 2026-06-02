#include "gameRoom.h"

#include "common/network/messages/server/world/EntitySpawnMessage.h"

GameRoom::GameRoom(uint32_t gameId, std::string gameName, uint8_t maxPlayers, NpcFactory& npcFactory,ItemRepository& itemRepo)
    : gameId(gameId),
      gameName(std::move(gameName)),
      maxPlayers(maxPlayers),
      monitor(),
      gameQueue(),
      world("assets/sprites/MapAssets/mapa.argmap",npcFactory, itemRepo),
      gameLoop(gameQueue, monitor, world)
{}

void GameRoom::addClient(uint32_t clientId,
                         Queue<std::shared_ptr<const Message>>& clientQueue) {
    monitor.addQueue(clientId, clientQueue);
}

void GameRoom::addPlayer(Player player) {
    world.addPlayer(std::move(player));
}

void GameRoom::removeClient(uint32_t clientId) {
    monitor.removeQueue(clientId);
    world.removePlayer(clientId);
}

Queue<ClientMessage>& GameRoom::getGameQueue() { return gameQueue; }

uint32_t GameRoom::getId() const { return gameId; }

const std::string& GameRoom::getName() const { return gameName; }

uint8_t GameRoom::getPlayerCount() const { return monitor.size(); }

uint8_t GameRoom::getMaxPlayers() const { return maxPlayers; }

bool GameRoom::isFull() const { return monitor.size() >= maxPlayers; }

void GameRoom::start() { gameLoop.start();  }

void GameRoom::stop()  { gameLoop.stop();  }

void GameRoom::join()  { gameLoop.join();}

void GameRoom::sendExistingPlayersTo(uint32_t newClientId) {
    for (const auto& [playerId, player] : world.getPlayers()) {
        if (playerId == newClientId) {
            continue;
        }

        PlayerDto dto = buildPlayerDto(player);

        monitor.sendTo(
            newClientId,
            std::make_shared<const EntitySpawnMessage>(std::move(dto))
        );
    }
}

void GameRoom::broadcastPlayerSpawn(uint32_t playerId) {
    const Player& player = world.getPlayer(playerId);

    PlayerDto dto = buildPlayerDto(player);

    monitor.broadcast(
        std::make_shared<const EntitySpawnMessage>(std::move(dto))
    );
}

PlayerDto GameRoom::buildPlayerDto(const Player& player) const {
    PlayerDto dto{};

    // Identidad del jugador.
    dto.playerID = static_cast<uint8_t>(player.getClientId());

    // Nombre del personaje.
    dto.nombre = player.getName();

    // Raza y clase.
    // Si estos campos no existen como .name, abajo te digo cómo resolverlo.
    dto.raza = player.getRace().name;
    std::cout << "[SERVER DTO] raza='" << dto.raza << "'" << std::endl;
    dto.clase = player.getCls().name;
    std::cout << "[SERVER DTO] clase='" << dto.clase << "'" << std::endl;

    // Apariencia inicial.
    dto.headId = 0;

    // Progresión.
    dto.level = player.getLevel();

    // Vida y maná.
    dto.hp = player.getHp();
    dto.hpMax = player.getMaxHp();
    dto.mana = player.getMana();
    dto.manaMax = player.getMaxMana();

    // Economía.
    dto.oro = static_cast<int>(player.getGold());
    dto.oroMax = 0;

    // Posición.
    dto.xpos = static_cast<uint16_t>(player.getPixelX());
    dto.ypos = static_cast<uint16_t>(player.getPixelY());

    // Experiencia.
    dto.exp = static_cast<int>(player.getExp());
    dto.expMax = 1000;

    // Estado lógico.
    dto.esFantasma = player.isGhost();

    // Atributos.
    dto.fuerza = player.getStrength();
    dto.agilidad = player.getAgility();
    dto.inteligencia = 10;
    dto.constitucion = 10;

    return dto;
}

void GameRoom::syncPlayerJoin(uint32_t newPlayerId) {
    // Al jugador nuevo le enviamos los jugadores que ya estaban en la sala.
    sendExistingPlayersTo(newPlayerId);

    // A todos los clientes les avisamos que existe el nuevo jugador.
    // El cliente local debe ignorar su propio spawn.
    broadcastPlayerSpawn(newPlayerId);
}