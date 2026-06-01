#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <vector>

#include "common/network/messages/message.h"
#include "common/network/messages/server/auth/connectOKMessage.h"
#include "common/queue.h"
#include "server/game/session/gameManager.h"
#include "server/monitorQueues.h"
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <toml++/toml.h>
#include <vector>

class GameManagerIntegrationTest : public ::testing::Test {
protected:
  toml::table config = toml::parse_file("config/game.toml");
  NpcRepository npcRepo{config};
  NpcFactory npcFactory{npcRepo};
  ItemRepository itemRepo{config};
  Queue<std::shared_ptr<LeaveEvent>> leaveQueue;
  GameManager gm{npcFactory, itemRepo, leaveQueue, config};
};

TEST_F(GameManagerIntegrationTest, CreateGameReturnsIncrementalIds) {
  uint32_t id1 = gm.createGame("sala1", 4);
  uint32_t id2 = gm.createGame("sala2", 4);
  EXPECT_NE(id1, id2);
  EXPECT_LT(id1, id2);
  gm.stopAll();
}

TEST_F(GameManagerIntegrationTest, ListGamesReflectsCreatedRooms) {
  gm.createGame("partida1", 4);
  gm.createGame("partida2", 2);
  auto games = gm.listGames();
  EXPECT_EQ(games.size(), 2u);
  gm.stopAll();
}

TEST_F(GameManagerIntegrationTest, ListGamesShowsCorrectMaxPlayers) {
  gm.createGame("sala", 5);
  auto games = gm.listGames();
  ASSERT_EQ(games.size(), 1u);
  EXPECT_EQ(games[0].maxPlayers, 5);
  gm.stopAll();
}

TEST_F(GameManagerIntegrationTest, ListGamesInitialPlayerCountIsZero) {
  gm.createGame("sala", 4);
  auto games = gm.listGames();
  ASSERT_EQ(games.size(), 1u);
  EXPECT_EQ(games[0].playerCount, 0);
  gm.stopAll();
}

TEST_F(GameManagerIntegrationTest, JoinGameIncreasesPlayerCount) {
  uint32_t gameId = gm.createGame("sala", 4);
  Queue<std::shared_ptr<const Message>> clientQueue;
  EXPECT_TRUE(gm.joinGame(gameId, 1, clientQueue));
  auto games = gm.listGames();
  EXPECT_EQ(games[0].playerCount, 1);
  gm.stopAll();
}

TEST_F(GameManagerIntegrationTest, JoinNonExistentGameReturnsFalse) {
  Queue<std::shared_ptr<const Message>> clientQueue;
  EXPECT_FALSE(gm.joinGame(999, 1, clientQueue));
  gm.stopAll();
}

TEST_F(GameManagerIntegrationTest, JoinFullGameReturnsFalse) {
  uint32_t gameId = gm.createGame("sala", 2);
  Queue<std::shared_ptr<const Message>> q1, q2, q3;
  EXPECT_TRUE(gm.joinGame(gameId, 1, q1));
  EXPECT_TRUE(gm.joinGame(gameId, 2, q2));
  EXPECT_FALSE(gm.joinGame(gameId, 3, q3));
  gm.stopAll();
}

TEST_F(GameManagerIntegrationTest, RemoveClientDecreasesPlayerCount) {
  uint32_t gameId = gm.createGame("sala", 4);
  Queue<std::shared_ptr<const Message>> clientQueue;
  gm.joinGame(gameId, 1, clientQueue);
  gm.removeClient(1);
  auto games = gm.listGames();
  EXPECT_EQ(games[0].playerCount, 0);
  gm.stopAll();
}

TEST_F(GameManagerIntegrationTest, RemoveNonExistentClientDoesNotCrash) {
  EXPECT_NO_THROW(gm.removeClient(999));
  gm.stopAll();
}

TEST_F(GameManagerIntegrationTest, ConcurrentJoinsRespectMaxPlayers) {
  uint32_t gameId = gm.createGame("sala", 5);
  constexpr int NUM_CLIENTS = 20;
  std::vector<Queue<std::shared_ptr<const Message>>> queues(NUM_CLIENTS);
  std::vector<bool> results(NUM_CLIENTS, false);
  std::vector<std::thread> threads;

  for (int i = 0; i < NUM_CLIENTS; ++i)
    threads.emplace_back(
        [&, i]() { results[i] = gm.joinGame(gameId, i + 1, queues[i]); });

  for (auto &t : threads)
    t.join();

  int accepted = 0;
  for (bool r : results)
    if (r)
      ++accepted;
  EXPECT_EQ(accepted, 5);
  gm.stopAll();
}

TEST(ClientRegistryTest, AddAndGetQueue) {
  Monitor registry;
  Queue<std::shared_ptr<const Message>> q;

  registry.addQueue(1, q);
  EXPECT_EQ(registry.getQueue(1), &q);
}

TEST(ClientRegistryTest, GetNonExistentReturnsNullptr) {
  Monitor registry;
  EXPECT_EQ(registry.getQueue(999), nullptr);
}

TEST(ClientRegistryTest, RemoveThenGetReturnsNullptr) {
  Monitor registry;
  Queue<std::shared_ptr<const Message>> q;

  registry.addQueue(1, q);
  registry.removeQueue(1);
  EXPECT_EQ(registry.getQueue(1), nullptr);
}

TEST(ClientRegistryTest, ConcurrentAddAndGet) {
  Monitor registry;
  constexpr int N = 50;
  std::vector<Queue<std::shared_ptr<const Message>>> queues(N);
  std::vector<std::thread> threads;

  for (int i = 0; i < N; ++i) {
    threads.emplace_back([&, i]() { registry.addQueue(i, queues[i]); });
  }

  for (auto &t : threads)
    t.join();

  for (int i = 0; i < N; ++i)
    EXPECT_EQ(registry.getQueue(i), &queues[i]);
}

TEST(ClientRegistryTest, ConcurrentAddAndRemove) {
  Monitor registry;
  constexpr int N = 50;
  std::vector<Queue<std::shared_ptr<const Message>>> queues(N);
  std::vector<std::thread> addThreads, removeThreads;

  for (int i = 0; i < N; ++i)
    registry.addQueue(i, queues[i]);

  for (int i = 0; i < N; ++i) {
    removeThreads.emplace_back([&, i]() { registry.removeQueue(i); });
  }

  for (auto &t : removeThreads)
    t.join();

  for (int i = 0; i < N; ++i)
    EXPECT_EQ(registry.getQueue(i), nullptr);
}

TEST(MonitorTest, SendToDeliversMessage) {
  Monitor monitor;
  Queue<std::shared_ptr<const Message>> q;
  monitor.addQueue(1, q);

  auto msg = std::make_shared<const ConnectOkMessage>();
  monitor.sendTo(1, msg);

  std::shared_ptr<const Message> received;
  EXPECT_TRUE(q.try_pop(received));
  EXPECT_EQ(received->opCode(), msg->opCode());
}

TEST(MonitorTest, SendToUnknownClientDoesNotCrash) {
  Monitor monitor;
  auto msg = std::make_shared<const ConnectOkMessage>();
  EXPECT_NO_THROW(monitor.sendTo(999, msg));
}

TEST(MonitorTest, BroadcastDeliversToAll) {
  Monitor monitor;
  Queue<std::shared_ptr<const Message>> q1, q2, q3;
  monitor.addQueue(1, q1);
  monitor.addQueue(2, q2);
  monitor.addQueue(3, q3);

  auto msg = std::make_shared<const ConnectOkMessage>();
  monitor.broadcast(msg);

  std::shared_ptr<const Message> r1, r2, r3;
  EXPECT_TRUE(q1.try_pop(r1));
  EXPECT_TRUE(q2.try_pop(r2));
  EXPECT_TRUE(q3.try_pop(r3));
}

TEST(MonitorTest, RemoveQueueStopsSendTo) {
  Monitor monitor;
  Queue<std::shared_ptr<const Message>> q;
  monitor.addQueue(1, q);
  monitor.removeQueue(1);

  auto msg = std::make_shared<const ConnectOkMessage>();
  monitor.sendTo(1, msg);

  std::shared_ptr<const Message> received;
  EXPECT_FALSE(q.try_pop(received));
}

TEST(MonitorTest, SizeReflectsAddAndRemove) {
  Monitor monitor;
  Queue<std::shared_ptr<const Message>> q1, q2;

  EXPECT_EQ(monitor.size(), 0);
  monitor.addQueue(1, q1);
  EXPECT_EQ(monitor.size(), 1);
  monitor.addQueue(2, q2);
  EXPECT_EQ(monitor.size(), 2);
  monitor.removeQueue(1);
  EXPECT_EQ(monitor.size(), 1);
}

TEST(MonitorTest, ConcurrentSendTo) {
  Monitor monitor;
  constexpr int N = 10;
  std::vector<Queue<std::shared_ptr<const Message>>> queues(N);

  for (int i = 0; i < N; ++i)
    monitor.addQueue(i, queues[i]);

  std::vector<std::thread> threads;
  for (int i = 0; i < N; ++i) {
    threads.emplace_back([&, i]() {
      auto msg = std::make_shared<const ConnectOkMessage>();
      monitor.sendTo(i, msg);
    });
  }

  for (auto &t : threads)
    t.join();

  for (int i = 0; i < N; ++i) {
    std::shared_ptr<const Message> r;
    EXPECT_TRUE(queues[i].try_pop(r));
  }
}