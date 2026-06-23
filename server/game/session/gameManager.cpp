#include "gameManager.h"

GameManager::GameManager(
    NpcFactory &npcFactory, ItemRepository &itemRepo,
    Queue<std::shared_ptr<LeaveEvent>> &leaveQueue,
    Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQueue,
    const toml::table &config, PlayerArchive &archive,
    GameArchive &gameArchive, ClanArchive &clanArchive,
    CharacterArchive &characterArchive)
    : npcFactory(npcFactory), itemRepo(itemRepo), leaveQueue(leaveQueue), transitionQueue(transitionQueue),
      config(config), archive(archive), clanManager(clanArchive, characterArchive, config), gameArchive(gameArchive)
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
    throw std::runtime_error("Empty map path received from client.");
  }

  {
    std::ifstream check(mapPath, std::ios::binary);
    if (!check.good())
    {
      throw std::runtime_error("Map not found: " + mapPath);
    }
  }

  uint32_t id = nextGameId++;
  std::unique_ptr<GameRoom> room = std::make_unique<GameRoom>(
      id, gameName, mapPath, false, 0,
      npcFactory, itemRepo, leaveQueue, transitionQueue,
      config, archive, clanManager);
  room->start();
  rooms.emplace(id, std::move(room));
  gameArchive.save(id, gameName, mapPath, maxPlayers);
  return id;
}

uint32_t GameManager::getOrCreateInstance(const std::string &mapPath,
                                          uint32_t originRoomId)
{
  std::unique_lock<std::mutex> lock(mutex);

  for (const std::pair<const uint32_t, std::unique_ptr<GameRoom>> &entry :
       rooms)
  {
    if (entry.second->getIsInstance() &&
        entry.second->getName() == mapPath &&
        entry.second->getOriginRoomId() == originRoomId)
    {
      return entry.first;
    }
  }

  {
    std::ifstream check(mapPath, std::ios::binary);
    if (!check.good())
    {
      throw std::runtime_error("Instance map not found: " + mapPath);
    }
  }

  uint32_t id = nextGameId++;
  std::unique_ptr<GameRoom> room = std::make_unique<GameRoom>(
      id, mapPath, mapPath, true, originRoomId,
      npcFactory, itemRepo, leaveQueue, transitionQueue,
      config, archive, clanManager);
  room->start();
  rooms.emplace(id, std::move(room));
  return id;
}

void GameManager::cleanEmptyInstances()
{
  std::vector<uint32_t> toRemove;
  for (const std::pair<const uint32_t, std::unique_ptr<GameRoom>> &entry :
       rooms)
  {
    if (entry.second->getIsInstance() &&
        entry.second->getPlayerCount() == 0)
    {
      toRemove.push_back(entry.first);
    }
  }

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

  std::unordered_map<uint32_t, std::unique_ptr<GameRoom>>::iterator it =
      rooms.find(gameId);
  if (it == rooms.end() || it->second->isFull())
  {
    return false;
  }

  it->second->addClient(clientId, clientQueue);
  clientRoom[clientId] = gameId;
  return true;
}

void GameManager::addPlayerToGame(uint32_t gameId, Player player)
{
  std::unique_lock<std::mutex> lock(mutex);

  const uint32_t playerId = player.getClientId();
  const std::string playerName = player.getName();

  std::optional<std::pair<std::string, bool>> clanInfo =
      clanManager.findClanInfoForMember(playerName);
  if (clanInfo.has_value())
  {
    player.setClanName(clanInfo->first);
    player.setClanFounder(clanInfo->second);
  }

  std::unordered_map<uint32_t, std::unique_ptr<GameRoom>>::iterator it =
      rooms.find(gameId);
  if (it == rooms.end())
  {
    std::cerr << "[GameManager] addPlayerToGame: unknown gameId="
              << gameId << std::endl;
    return;
  }

  it->second->addPlayer(std::move(player));
  clientNick[playerId] = playerName;
  nickToClient[playerName] = playerId;
  clientRoom[playerId] = gameId;

  if (clanInfo.has_value())
  {
    const std::string &clanName = clanInfo->first;
    std::shared_ptr<ChatNotificationMessage> chatMsg =
        std::make_shared<ChatNotificationMessage>(
            "Your ally " + playerName + " has entered Argentum.",
            ChatMsgType::CLAN);

    for (const std::pair<const uint32_t, uint32_t> &entry : clientRoom)
    {
      if (entry.second != gameId)
        continue;

      if (entry.first == playerId)
        continue;

      std::unordered_map<uint32_t, std::string>::const_iterator
          targetNickIt = clientNick.find(entry.first);
      if (targetNickIt == clientNick.end())
        continue;

      std::optional<std::pair<std::string, bool>> targetClan =
          clanManager.findClanInfoForMember(targetNickIt->second);
      if (targetClan.has_value() && targetClan->first == clanName)
      {
        it->second->sendTo(entry.first, chatMsg);
      }
    }
  }
}

std::vector<GameInfo> GameManager::listGames() const
{
  std::unique_lock<std::mutex> lock(mutex);
  std::vector<GameInfo> result;
  for (const std::pair<const uint32_t, std::unique_ptr<GameRoom>> &entry :
       rooms)
  {
    if (entry.second->getIsInstance())
    {
      continue;
    }
    GameInfo info;
    info.gameId = entry.second->getId();
    info.gameName = entry.second->getName();
    info.playerCount = entry.second->getPlayerCount();
    info.maxPlayers = entry.second->getMaxPlayers();
    info.mapPath = entry.second->getMapPath();
    result.push_back(std::move(info));
  }
  return result;
}

void GameManager::stopAll()
{
  std::unique_lock<std::mutex> lock(mutex);
  for (std::pair<const uint32_t, std::unique_ptr<GameRoom>> &entry : rooms)
  {
    entry.second->stop();
    entry.second->join();
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
  std::unordered_map<uint32_t, std::unique_ptr<GameRoom>>::const_iterator it =
      rooms.find(instanceRoomId);
  if (it == rooms.end())
  {
    return 0;
  }
  return it->second->getOriginRoomId();
}

void GameManager::broadcastExceptInGame(
    uint32_t gameId, uint32_t excludeId,
    const std::shared_ptr<const Message> &msg)
{
  std::unique_lock<std::mutex> lock(mutex);
  std::unordered_map<uint32_t, std::unique_ptr<GameRoom>>::iterator it =
      rooms.find(gameId);
  if (it != rooms.end())
  {
    it->second->broadcastExcept(excludeId, msg);
  }
}

const GameWorld *GameManager::getGameWorld(uint32_t gameId) const
{
  std::unique_lock<std::mutex> lock(mutex);
  std::unordered_map<uint32_t, std::unique_ptr<GameRoom>>::const_iterator it =
      rooms.find(gameId);
  if (it == rooms.end())
  {
    return nullptr;
  }
  return &it->second->getWorld();
}

void GameManager::syncPlayerJoin(uint32_t gameId, uint32_t playerId)
{
  std::unique_lock<std::mutex> lock(mutex);
  std::unordered_map<uint32_t, std::unique_ptr<GameRoom>>::iterator it =
      rooms.find(gameId);
  if (it != rooms.end())
  {
    it->second->syncPlayerJoin(playerId);
  }
}

void GameManager::restoreFromArchive()
{
  clanManager.restoreFromArchive();

  std::vector<GameRecord> records = gameArchive.loadAll();
  uint32_t maxId = gameArchive.maxGameId();
  if (maxId >= nextGameId)
  {
    nextGameId = maxId + 1;
  }

  for (const GameRecord &rec : records)
  {
    std::string mapPath(rec.mapPath,
                        strnlen(rec.mapPath, sizeof(rec.mapPath)));
    std::string gameName(rec.gameName,
                         strnlen(rec.gameName, sizeof(rec.gameName)));

    std::ifstream check(mapPath, std::ios::binary);
    if (!check.good())
    {
      std::cerr << "[GameManager] restore: map not found for gameId="
                << rec.gameId << " path='" << mapPath
                << "' — skipping." << std::endl;
      continue;
    }

    std::unique_ptr<GameRoom> room = std::make_unique<GameRoom>(
        rec.gameId, gameName, mapPath, false, 0,
        npcFactory, itemRepo, leaveQueue, transitionQueue,
        config, archive, clanManager);
    room->start();
    rooms.emplace(rec.gameId, std::move(room));
  }
}

std::string GameManager::getRoomMapPath(uint32_t gameId) const
{
  std::unique_lock<std::mutex> lock(mutex);
  std::unordered_map<uint32_t, std::unique_ptr<GameRoom>>::const_iterator it =
      rooms.find(gameId);
  if (it == rooms.end())
  {
    return "";
  }
  if (it->second->getIsInstance())
  {
    return it->second->getName();
  }
  return it->second->getMapPath();
}

bool GameManager::tryMarkOnline(uint32_t clientId,
                                const std::string &characterName)
{
  std::unique_lock<std::mutex> lock(mutex);
  if (onlineCharacters.find(characterName) != onlineCharacters.end())
  {
    return false;
  }
  onlineCharacters.insert(characterName);
  clientToCharacter[clientId] = characterName;
  return true;
}

void GameManager::markOffline(uint32_t clientId)
{
  std::unique_lock<std::mutex> lock(mutex);
  std::unordered_map<uint32_t, std::string>::iterator it =
      clientToCharacter.find(clientId);
  if (it == clientToCharacter.end())
  {
    return;
  }
  onlineCharacters.erase(it->second);
  clientToCharacter.erase(it);
}

void GameManager::removeClient(uint32_t clientId)
{
  markOffline(clientId);
  std::unique_lock<std::mutex> lock(mutex);

  std::unordered_map<uint32_t, uint32_t>::iterator it =
      clientRoom.find(clientId);
  if (it == clientRoom.end())
  {
    return;
  }

  uint32_t gameId = it->second;
  clientRoom.erase(it);

  std::string playerName;
  std::string playerClan;
  bool hadClan = false;

  std::unordered_map<uint32_t, std::unique_ptr<GameRoom>>::iterator roomIt =
      rooms.find(gameId);
  if (roomIt != rooms.end())
  {
    const Player *player = roomIt->second->findPlayer(clientId);
    if (player != nullptr)
    {
      playerName = player->getName();
      std::optional<std::pair<std::string, bool>> clanInfo =
          clanManager.findClanInfoForMember(playerName);
      if (clanInfo.has_value())
      {
        playerClan = clanInfo->first;
        hadClan = true;
      }
    }
    roomIt->second->removeClient(clientId);
  }

  std::unordered_map<uint32_t, std::string>::iterator nickIt =
      clientNick.find(clientId);
  if (nickIt != clientNick.end())
  {
    nickToClient.erase(nickIt->second);
    clientNick.erase(nickIt);
  }

  cleanEmptyInstances();

  if (hadClan && roomIt != rooms.end())
  {
    std::shared_ptr<ChatNotificationMessage> chatMsg =
        std::make_shared<ChatNotificationMessage>(
            "Your ally " + playerName + " has left Argentum.",
            ChatMsgType::CLAN);

    for (const std::pair<const uint32_t, uint32_t> &entry : clientRoom)
    {
      if (entry.second != gameId)
      {
        continue;
      }
      std::unordered_map<uint32_t, std::string>::const_iterator
          targetNickIt = clientNick.find(entry.first);
      if (targetNickIt == clientNick.end())
      {
        continue;
      }
      std::optional<std::pair<std::string, bool>> targetClan =
          clanManager.findClanInfoForMember(targetNickIt->second);
      if (targetClan.has_value() && targetClan->first == playerClan)
      {
        roomIt->second->sendTo(entry.first, chatMsg);
      }
    }
  }
}

void GameManager::sendToClient(uint32_t clientId,
                               const std::shared_ptr<const Message> &msg)
{
  std::unique_lock<std::mutex> lock(mutex);
  std::unordered_map<uint32_t, uint32_t>::const_iterator roomIt =
      clientRoom.find(clientId);
  if (roomIt == clientRoom.end())
  {
    return;
  }
  std::unordered_map<uint32_t, std::unique_ptr<GameRoom>>::iterator it =
      rooms.find(roomIt->second);
  if (it != rooms.end())
  {
    it->second->sendTo(clientId, msg);
  }
}

std::optional<uint32_t> GameManager::findOnlineClientByNick(
    const std::string &nick) const
{
  std::unique_lock<std::mutex> lock(mutex);
  std::unordered_map<std::string, uint32_t>::const_iterator it =
      nickToClient.find(nick);
  if (it == nickToClient.end())
  {
    return std::nullopt;
  }
  return it->second;
}

void GameManager::updatePlayerClanState(const std::string &nick,
                                        const std::string &clanName,
                                        bool isFounder)
{
  std::unique_lock<std::mutex> lock(mutex);

  std::unordered_map<std::string, uint32_t>::const_iterator it =
      nickToClient.find(nick);
  if (it == nickToClient.end())
  {
    return;
  }

  uint32_t clientId = it->second;
  std::unordered_map<uint32_t, uint32_t>::const_iterator roomIt =
      clientRoom.find(clientId);
  if (roomIt == clientRoom.end())
  {
    return;
  }

  std::unordered_map<uint32_t, std::unique_ptr<GameRoom>>::iterator
      roomPtrIt = rooms.find(roomIt->second);
  if (roomPtrIt == rooms.end())
  {
    return;
  }

  ClientMessage syncMsg{
      clientId,
      std::make_shared<ClanSyncMessage>(clanName, isFounder)};
  roomPtrIt->second->getGameQueue().try_push(syncMsg);
}

void GameManager::unregisterClientForTransition(uint32_t clientId)
{
  std::unique_lock<std::mutex> lock(mutex);

  std::unordered_map<uint32_t, uint32_t>::iterator roomIt =
      clientRoom.find(clientId);
  if (roomIt == clientRoom.end())
  {
    return;
  }

  uint32_t oldGameId = roomIt->second;
  clientRoom.erase(roomIt);

  std::unordered_map<uint32_t, std::string>::iterator nickIt =
      clientNick.find(clientId);
  if (nickIt != clientNick.end())
  {
    nickToClient.erase(nickIt->second);
    clientNick.erase(nickIt);
  }

  std::unordered_map<uint32_t, std::unique_ptr<GameRoom>>::iterator it =
      rooms.find(oldGameId);
  if (it != rooms.end())
  {
    it->second->removeMonitorOnly(clientId);
  }
}

void GameManager::broadcastDespawnInRoom(uint32_t fromRoomId,
                                         uint32_t clientId)
{
  std::unique_lock<std::mutex> lock(mutex);
  std::unordered_map<uint32_t, std::unique_ptr<GameRoom>>::iterator it =
      rooms.find(fromRoomId);
  if (it == rooms.end())
  {
    return;
  }
  it->second->broadcastExcept(clientId,
                              std::make_shared<const EntityDespawnMessage>(clientId));
}

void GameManager::joinAndAddPlayer(
    uint32_t gameId, uint32_t clientId,
    Queue<std::shared_ptr<const Message>> &clientQueue,
    Player player)
{
  std::unique_lock<std::mutex> lock(mutex);

  std::unordered_map<uint32_t, std::unique_ptr<GameRoom>>::iterator it =
      rooms.find(gameId);
  if (it == rooms.end())
  {
    return;
  }

  const std::string playerName = player.getName();
  const uint32_t playerId = player.getClientId();

  std::optional<std::pair<std::string, bool>> clanInfo =
      clanManager.findClanInfoForMember(playerName);
  if (clanInfo.has_value())
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