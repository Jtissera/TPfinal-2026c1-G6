#include "gameLoop.h"

GameLoop::GameLoop(Queue<ClientMessage> &q, Monitor &m, GameWorld &w,
                   Queue<std::shared_ptr<LeaveEvent>> &leaveQ, uint32_t gameId,
                   const toml::table &config)
    : gameQueue(q), monitor(m), world(w), leaveQueue(leaveQ), gameId(gameId),
      dispatcher(config),
      tickRateMs(config["server"]["tick_rate_ms"].value_or(33)) {}

void GameLoop::run() {
  using Clock = std::chrono::steady_clock;
  using Ms = std::chrono::duration<float, std::milli>;
  using Duration = std::chrono::milliseconds;

  auto t1 = Clock::now();

  try {
    while (true) {
      ClientMessage incoming;
      while (gameQueue.try_pop(incoming))
        processMessage(incoming);

      auto t2 = Clock::now();
      float elapsed = std::chrono::duration_cast<Ms>(t2 - t1).count();
      float rest = tickRateMs - elapsed;

      if (rest < 0) {
        float behind = -rest;
        rest = tickRateMs - std::fmod(behind, tickRateMs);
        float lost = behind + rest;
        t1 += Duration(static_cast<long>(lost));
      } else {
        std::this_thread::sleep_for(Duration(static_cast<long>(rest)));
      }

      worldUpdate(tickRateMs / 1000.0f);
      t1 += Duration(static_cast<long>(tickRateMs));
    }
  } catch (const ClosedQueue &) {
  } catch (const std::exception &e) {
    std::cerr << "[GameLoop] error: " << e.what() << std::endl;
  }
}

void GameLoop::processMessage(const ClientMessage &incoming) {
  if (incoming.message->opCode() ==
      static_cast<uint8_t>(ClientOpCode::MSG_LEAVE_GAME)) {
    handleLeaveGame(incoming.clientId);
    return;
  }
  dispatcher.dispatch(incoming, world, monitor);
}

void GameLoop::worldUpdate(float deltaSeconds) {
  auto result = world.tick(deltaSeconds);
  for (uint32_t id : result.playersChanged)
    statManager.sendPlayerStats(id, world, monitor);
}

void GameLoop::handleLeaveGame(uint32_t clientId) {
  auto player = world.removePlayer(clientId);
  if (!player)
    return;

  Queue<std::shared_ptr<const Message>> *clientQueue =
      monitor.getQueue(clientId);
  monitor.removeQueue(clientId);

  leaveQueue.try_push(std::make_shared<LeaveEvent>(
      LeaveEvent{clientId, gameId, std::move(*player), clientQueue}));
}

void GameLoop::stop() {
  Thread::stop();
  gameQueue.close();
}