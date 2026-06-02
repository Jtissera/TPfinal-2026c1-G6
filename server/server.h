#pragma once

#include "../common/network/sockets.h"
#include "../common/queue.h"
#include "clientMessage.h"
#include "game/items/itemRepository.h"
#include "game/player/playerFactory.h"
#include "game/session/gameManager.h"
#include "game/stats/classRepository.h"
#include "game/stats/raceRepository.h"
#include "lobby/leaveEvent.h"
#include "lobby/lobbyHandler.h"
#include "lobby/playerRepository.h"
#include "monitorQueues.h"
#include "network/acceptor.h"
#include "network/receiverRegistry.h"
#include "npc/npcRepository.h"
#include <toml++/toml.h>

class Server
{
public:
  explicit Server(const char *servname);
  int run();

private:
  toml::table config;
  ClassRepository classRepo;
  RaceRepository raceRepo;
  PlayerFactory playerFactory;
  PlayerRepository playerRepo;

  Monitor lobbyMonitor;
  Queue<ClientMessage> lobbyQueue;

  Monitor clientRegistry;
  ReceiverRegistry receiverRegistry;
  Queue<std::shared_ptr<LeaveEvent>> leaveQueue;
  Queue<std::shared_ptr<InstanceTransitionEvent>> transitionQueue;
  GameManager gameManager;
  LobbyHandler lobbyHandler;

  Socket socket;
  Acceptor acceptor;

  NpcRepository npcRepo;
  ItemRepository itemRepo;
  NpcFactory npcFactory;
};