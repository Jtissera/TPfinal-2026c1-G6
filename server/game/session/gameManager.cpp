#include "gameManager.h"
#include "common/network/messages/internal/clanSyncMessage.h"

GameManager::GameManager(
    NpcFactory &npcFactory, ItemRepository &itemRepo,
    Queue<std::shared_ptr<LeaveEvent>> &leaveQueue,
    Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQueue,
    const toml::table &config, PlayerArchive &archive, GameArchive &gameArchive,
    ClanArchive &clanArchive, CharacterArchive &characterArchive)
    : npcFactory(npcFactory), itemRepo(itemRepo), leaveQueue(leaveQueue),
      transitionQueue(transitionQueue), config(config), archive(archive),
      clanManager(clanArchive, characterArchive),
      gameArchive(gameArchive)
{
  clanManager.bindGameManager(this);
}


uint32_t GameManager::createGame(const std::string &gameName,
                                 uint8_t maxPlayers,
                                 const std::string &mapPath)
{
  std::unique_lock<std::mutex> lock(mutex);

  if (mapPath.empty())
  {
    throw std::runtime_error(
        "Error crítico: El cliente envió una ruta de mapa vacía.");
  }

  {
    std::ifstream check(mapPath, std::ios::binary);
    if (!check.good())
      throw std::runtime_error("Mapa no encontrado: " + mapPath);
  }

  uint32_t id = nextGameId++;

  auto room = std::make_unique<GameRoom>(id, gameName, mapPath, false, 0,
                                         npcFactory, itemRepo, leaveQueue,
                                         transitionQueue, config, archive, clanManager);
  room->start();
  rooms.emplace(id, std::move(room));

  std::cout << "[GameManager] createGame id=" << id << " name='" << gameName
            << "'"
            << " map='" << mapPath << "'" << std::endl;

  gameArchive.save(id, gameName, mapPath, maxPlayers);

  return id;
}

uint32_t GameManager::getOrCreateInstance(const std::string &mapPath,
                                          uint32_t originRoomId)
{
  std::unique_lock<std::mutex> lock(mutex);

  for (const auto &[id, room] : rooms)
  {
    if (room->getIsInstance() && room->getName() == mapPath &&
        room->getOriginRoomId() == originRoomId)
    {
      return id;
    }
  }

  {
    std::ifstream check(mapPath, std::ios::binary);
    if (!check.good())
      throw std::runtime_error("Mapa de instancia no encontrado: " + mapPath);
  }

  uint32_t id = nextGameId++;
  auto room = std::make_unique<GameRoom>(
      id, mapPath, mapPath, true, originRoomId, npcFactory, itemRepo,
      leaveQueue, transitionQueue, config, archive, clanManager);
  room->start();
  rooms.emplace(id, std::move(room));
  return id;
}

void GameManager::cleanEmptyInstances()
{
  std::vector<uint32_t> toRemove;
  for (const auto &[id, room] : rooms)
    if (room->getIsInstance() && room->getPlayerCount() == 0)
      toRemove.push_back(id);

  for (uint32_t id : toRemove)
  {
    rooms[id]->stop();
    rooms[id]->join();
    rooms.erase(id);
  }
}

bool GameManager::joinGame(uint32_t gameId, uint32_t clientId,
                           Queue<std::shared_ptr<const Message>> &clientQueue)
{
  std::unique_lock<std::mutex> lock(mutex);

  auto it = rooms.find(gameId);
  if (it == rooms.end())
    return false;
  if (it->second->isFull())
    return false;

  it->second->addClient(clientId, clientQueue);
  clientRoom[clientId] = gameId;

  std::cout << "[GameManager] client=" << clientId << " joined game=" << gameId
            << std::endl;

  return true;
}

void GameManager::addPlayerToGame(uint32_t gameId, Player player)
{
  std::unique_lock<std::mutex> lock(mutex);

  const uint32_t playerId = player.getClientId();
  const std::string playerName = player.getName();

  std::cout << "[GameManager] addPlayerToGame gameId="
            << gameId
            << " playerId="
            << playerId
            << std::endl;

  auto clanInfo = clanManager.findClanInfoForMember(playerName);
  if (clanInfo)
  {
    player.setClanName(clanInfo->first);
    player.setClanFounder(clanInfo->second);
  }

  auto it = rooms.find(gameId);

  if (it == rooms.end())
  {
    std::cerr << "[GameManager] addPlayerToGame fallo. gameId inexistente="
              << gameId
              << std::endl;
    return;
  }

  it->second->addPlayer(std::move(player));

  clientNick[playerId] = playerName;
  nickToClient[playerName] = playerId;
  clientRoom[playerId] = gameId;

  std::cout << "[GameManager] addPlayerToGame OK gameId="
            << gameId
            << " playerId="
            << playerId
            << std::endl;

  // 2. Notificación LOCAL: Avisamos SOLO a los miembros del clan en esta misma sala
  if (clanInfo)
  {
    std::string clanName = clanInfo->first;
    auto chatMsg = std::make_shared<ChatNotificationMessage>(
        "Tu aliado " + playerName + " ha ingresado a Argentum.", ChatMsgType::CLAN);

    for (const auto &[cId, rId] : clientRoom)
    {
      if (rId == gameId)
      {
        auto targetNickIt = clientNick.find(cId);
        if (targetNickIt != clientNick.end())
        {
          auto targetClan = clanManager.findClanInfoForMember(targetNickIt->second);
          if (targetClan && targetClan->first == clanName)
          {
            it->second->sendTo(cId, chatMsg);
          }
        }
      }
    }
  }
}

std::vector<GameInfo> GameManager::listGames() const
{
  std::unique_lock<std::mutex> lock(mutex);
  std::vector<GameInfo> result;
  for (const auto &[id, room] : rooms)
  {
    if (room->getIsInstance())
      continue; // oculto instancias
    GameInfo info;
    info.gameId = room->getId();
    info.gameName = room->getName();
    info.playerCount = room->getPlayerCount();
    info.maxPlayers = room->getMaxPlayers();
    info.mapPath = room->getMapPath();

    result.push_back(std::move(info));
  }
  return result;
}

void GameManager::stopAll()
{
  std::unique_lock<std::mutex> lock(mutex);

  for (auto &pair : rooms)
  {
    pair.second->stop();
    pair.second->join();
  }
  rooms.clear();
  clientRoom.clear();
}

Queue<ClientMessage> &GameManager::getGameQueue(uint32_t gameId)
{
  std::unique_lock<std::mutex> lock(mutex);
  return rooms.at(gameId)->getGameQueue();
}

uint32_t GameManager::getOriginRoomId(uint32_t instanceRoomId) const
{
  std::unique_lock<std::mutex> lock(mutex);
  auto it = rooms.find(instanceRoomId);
  if (it == rooms.end())
    return 0;
  return it->second->getOriginRoomId();
}
void GameManager::broadcastExceptInGame(
    uint32_t gameId, uint32_t excludeId,
    const std::shared_ptr<const Message> &msg)
{
  std::unique_lock<std::mutex> lock(mutex);
  auto it = rooms.find(gameId);
  if (it != rooms.end())
    it->second->broadcastExcept(excludeId, msg);
}

const GameWorld *GameManager::getGameWorld(uint32_t gameId) const
{
  std::unique_lock<std::mutex> lock(mutex);
  auto it = rooms.find(gameId);
  if (it == rooms.end())
    return nullptr;
  return &it->second->getWorld();
}

void GameManager::syncPlayerJoin(uint32_t gameId, uint32_t playerId)
{
  std::unique_lock<std::mutex> lock(mutex);

  auto it = rooms.find(gameId);

  if (it != rooms.end())
  {
    std::cout << "[GameManager] syncPlayerJoin gameId=" << gameId
              << " playerId=" << playerId << std::endl;

    it->second->syncPlayerJoin(playerId);
  }
}

void GameManager::restoreFromArchive()
{
  clanManager.restoreFromArchive();
 
  auto records = gameArchive.loadAll();
 
  uint32_t maxId = gameArchive.maxGameId();
  if (maxId >= nextGameId)
    nextGameId = maxId + 1;
 
  for (const auto &rec : records)
  {
    std::string mapPath(rec.mapPath, strnlen(rec.mapPath, sizeof(rec.mapPath)));
    std::string gameName(rec.gameName,
                         strnlen(rec.gameName, sizeof(rec.gameName)));
 
    std::ifstream check(mapPath, std::ios::binary);
    if (!check.good())
    {
      std::cerr << "[GameManager] restore: mapa no encontrado para gameId="
                << rec.gameId << " path='" << mapPath << "' — omitiendo."
                << std::endl;
      continue;
    }
 
    auto room = std::make_unique<GameRoom>(rec.gameId, gameName, mapPath, false,
                                           0, npcFactory, itemRepo, leaveQueue,
                                           transitionQueue, config, archive, clanManager);
    room->start();
    rooms.emplace(rec.gameId, std::move(room));
 
    std::cout << "[GameManager] restore gameId=" << rec.gameId << " name='"
              << gameName << "'" << std::endl;
  }
}

std::string GameManager::getRoomMapPath(uint32_t gameId) const
{
  std::unique_lock<std::mutex> lock(mutex);
  auto it = rooms.find(gameId);
  if (it == rooms.end())
    return "";

  if (it->second->getIsInstance())
  {
    return it->second->getName();
  }
  else
  {
    return it->second->getMapPath();
  }
}



bool GameManager::tryMarkOnline(uint32_t clientId, const std::string &characterName)
{
  std::unique_lock<std::mutex> lock(mutex);

  if (onlineCharacters.find(characterName) != onlineCharacters.end())
  {
    return false; 
  }

  onlineCharacters.insert(characterName);
  clientToCharacter[clientId] = characterName;

  std::cout << "[GameManager] tryMarkOnline OK character='" << characterName
            << "' clientId=" << clientId << std::endl;

  return true;
}

void GameManager::markOffline(uint32_t clientId)
{
  std::unique_lock<std::mutex> lock(mutex);

  auto it = clientToCharacter.find(clientId);
  if (it == clientToCharacter.end())
    return;

  std::cout << "[GameManager] markOffline character='" << it->second
            << "' clientId=" << clientId << std::endl;

  onlineCharacters.erase(it->second);
  clientToCharacter.erase(it);
}

void GameManager::removeClient(uint32_t clientId)
{
  markOffline(clientId);

  std::unique_lock<std::mutex> lock(mutex);

  auto it = clientRoom.find(clientId);
  if (it == clientRoom.end())
    return;

  uint32_t gameId = it->second;
  clientRoom.erase(it);

  std::string playerName;
  std::string playerClan;
  bool hadClan = false;

  auto roomIt = rooms.find(gameId);
  if (roomIt != rooms.end())
  {
    const Player *p = roomIt->second->findPlayer(clientId);
    if (p != nullptr)
    {
      playerName = p->getName();
      auto clanInfo = clanManager.findClanInfoForMember(playerName);
      if (clanInfo)
      {
        playerClan = clanInfo->first;
        hadClan = true;
      }
    }
    roomIt->second->removeClient(clientId);
  }

  auto nickIt = clientNick.find(clientId);
  if (nickIt != clientNick.end())
  {
    nickToClient.erase(nickIt->second);
    clientNick.erase(nickIt);
  }

  cleanEmptyInstances();

  if (hadClan && roomIt != rooms.end())
  {
    auto chatMsg = std::make_shared<ChatNotificationMessage>(
        "Tu aliado " + playerName + " ha salido de Argentum.", ChatMsgType::CLAN);

    for (const auto &[cId, rId] : clientRoom)
    {
      if (rId == gameId)
      {
        auto targetNickIt = clientNick.find(cId);
        if (targetNickIt != clientNick.end())
        {
          auto targetClan = clanManager.findClanInfoForMember(targetNickIt->second);
          if (targetClan && targetClan->first == playerClan)
          {
            roomIt->second->sendTo(cId, chatMsg);
          }
        }
      }
    }
  }
}

void GameManager::sendToClient(uint32_t clientId, const std::shared_ptr<const Message> &msg)
{
  std::unique_lock<std::mutex> lock(mutex);
  auto roomIt = clientRoom.find(clientId);
  if (roomIt == clientRoom.end())
    return;
  auto it = rooms.find(roomIt->second);
  if (it != rooms.end())
    it->second->sendTo(clientId, msg);
}

std::optional<uint32_t> GameManager::findOnlineClientByNick(const std::string &nick) const
{
  std::unique_lock<std::mutex> lock(mutex);
  auto it = nickToClient.find(nick);
  if (it == nickToClient.end())
    return std::nullopt;
  return it->second;
}

void GameManager::updatePlayerClanState(const std::string &nick, const std::string &clanName, bool isFounder)
{
  std::unique_lock<std::mutex> lock(mutex);

  auto it = nickToClient.find(nick);
  if (it == nickToClient.end())
    return;

  uint32_t clientId = it->second;
  auto roomIt = clientRoom.find(clientId);
  if (roomIt == clientRoom.end())
    return;

  auto roomPtrIt = rooms.find(roomIt->second);
  if (roomPtrIt == rooms.end())
    return;

  ClientMessage syncMsg{clientId,
                        std::make_shared<ClanSyncMessage>(clanName, isFounder)};
  roomPtrIt->second->getGameQueue().try_push(syncMsg);
}

void GameManager::unregisterClientForTransition(uint32_t clientId)
{
  std::unique_lock<std::mutex> lock(mutex);

  auto roomIt = clientRoom.find(clientId);
  if (roomIt == clientRoom.end())
    return;

  uint32_t oldGameId = roomIt->second;
  clientRoom.erase(roomIt);

  auto nickIt = clientNick.find(clientId);
  if (nickIt != clientNick.end())
  {
    nickToClient.erase(nickIt->second);
    clientNick.erase(nickIt);
  }

  auto it = rooms.find(oldGameId);
  if (it != rooms.end())
    it->second->removeMonitorOnly(clientId);
}

void GameManager::broadcastDespawnInRoom(uint32_t fromRoomId, uint32_t clientId)
{
  std::unique_lock<std::mutex> lock(mutex);
  auto it = rooms.find(fromRoomId);
  if (it == rooms.end())
    return;
  it->second->broadcastExcept(clientId,
                              std::make_shared<const EntityDespawnMessage>(clientId));
}

void GameManager::joinAndAddPlayer(uint32_t gameId, uint32_t clientId,
                                   Queue<std::shared_ptr<const Message>> &clientQueue,
                                   Player player)
{
  std::unique_lock<std::mutex> lock(mutex);

  auto it = rooms.find(gameId);
  if (it == rooms.end())
    return;

  const std::string playerName = player.getName();
  const uint32_t playerId = player.getClientId();

  auto clanInfo = clanManager.findClanInfoForMember(playerName);
  if (clanInfo)
  {
    player.setClanName(clanInfo->first);
    player.setClanFounder(clanInfo->second);
  }

  it->second->addPlayer(std::move(player));

  it->second->addClient(clientId, clientQueue);

  clientNick[playerId] = playerName;
  nickToClient[playerName] = playerId;
  clientRoom[playerId] = gameId;
}