
#ifndef TALLER_TP_GAMECLIENT_H
#define TALLER_TP_GAMECLIENT_H
#pragma once
#include <memory>

#include "../common/network/sockets.h"
#include "ClientReceiver.h"
#include "ClientSender.h"
#include "Game.h"
#include "common/dtos/gameTypes.h"
#include "common/network/messages/message.h"
#include "common/queue.h"
#include "network/clientProtocolFactory.h"
class GameClient
{

public:
  GameClient(Socket &socket, uint32_t idPlayer, const PlayerDto &playerDto,
             SDL_Window *window, SDL_Renderer *renderer, const std::string &mapPath);
  void run();
  bool wasDisconnectedByServer() const { return connectionLost; }

private:
  Socket &socket;
  const uint32_t idPlayer;
  const PlayerDto playerDto;

  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;

  ClientProtocolFactory factory;

  Protocol senderProtocol;
  Protocol receiverProtocol;

  Queue<std::shared_ptr<const Message>> sendQueue;
  Queue<std::shared_ptr<const Message>> receiveQueue;

  ClientSender sender;
  ClientReceiver receiver;

  Game gameLoop;
  std::string mapPath;

  std::atomic<bool> connectionLost{false};
  bool checkSocketStatus();
};

#endif // TALLER_TP_GAMECLIENT_H
