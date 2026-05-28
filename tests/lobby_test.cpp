#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <memory>
#include <vector>

#include "server/gameManager.h"
#include "server/clientRegistry.h"
#include "server/monitorQueues.h"
#include "common/network/messages/message.h"
#include "common/queue.h"
#include "common/network/messages/server/auth/connectOKMessage.h"

TEST(GameManagerTest, CreateGameReturnsIncrementalIds)
{
    GameManager gm;
    
    uint32_t id1 = gm.createGame("sala1", 4);
    uint32_t id2 = gm.createGame("sala2", 4);
    
    std::cout << "--> Llegué a los EXPECT" << std::endl;
    EXPECT_NE(id1, id2);
    EXPECT_LT(id1, id2);
    
    std::cout << "--> Pasé los EXPECT, entrando a stopAll" << std::endl;
    gm.stopAll();
    
    std::cout << "--> Salí de stopAll" << std::endl;
}
TEST(GameManagerTest, ListGamesReflectsCreatedRooms)
{
    GameManager gm;
    gm.createGame("partida1", 4);
    gm.createGame("partida2", 2);

    auto games = gm.listGames();
    EXPECT_EQ(games.size(), 2u);
    gm.stopAll();
}

TEST(GameManagerTest, ListGamesShowsCorrectMaxPlayers)
{
    GameManager gm;
    gm.createGame("sala", 5);

    auto games = gm.listGames();
    ASSERT_EQ(games.size(), 1u);
    EXPECT_EQ(games[0].maxPlayers, 5);
    gm.stopAll();
}

TEST(GameManagerTest, ListGamesInitialPlayerCountIsZero)
{
    GameManager gm;
    gm.createGame("sala", 4);

    auto games = gm.listGames();
    ASSERT_EQ(games.size(), 1u);
    EXPECT_EQ(games[0].playerCount, 0);
    gm.stopAll();
}

TEST(GameManagerTest, JoinGameIncreasesPlayerCount)
{
    GameManager gm;
    uint32_t gameId = gm.createGame("sala", 4);

    Queue<std::shared_ptr<const Message>> clientQueue;
    bool joined = gm.joinGame(gameId, 1, clientQueue);

    EXPECT_TRUE(joined);
    auto games = gm.listGames();
    EXPECT_EQ(games[0].playerCount, 1);
    gm.stopAll();
}

TEST(GameManagerTest, JoinNonExistentGameReturnsFalse)
{
    GameManager gm;
    Queue<std::shared_ptr<const Message>> clientQueue;
    bool joined = gm.joinGame(999, 1, clientQueue);
    EXPECT_FALSE(joined);
    gm.stopAll();
}

TEST(GameManagerTest, JoinFullGameReturnsFalse)
{
    GameManager gm;
    uint32_t gameId = gm.createGame("sala", 2);

    Queue<std::shared_ptr<const Message>> q1, q2, q3;
    EXPECT_TRUE(gm.joinGame(gameId, 1, q1));
    EXPECT_TRUE(gm.joinGame(gameId, 2, q2));
    EXPECT_FALSE(gm.joinGame(gameId, 3, q3)); // sala llena
    gm.stopAll();
}

TEST(GameManagerTest, MultiplePlayersJoinDifferentRooms)
{
    GameManager gm;
    uint32_t game1 = gm.createGame("sala1", 4);
    uint32_t game2 = gm.createGame("sala2", 4);

    Queue<std::shared_ptr<const Message>> q1, q2, q3, q4;
    EXPECT_TRUE(gm.joinGame(game1, 1, q1));
    EXPECT_TRUE(gm.joinGame(game1, 2, q2));
    EXPECT_TRUE(gm.joinGame(game2, 3, q3));
    EXPECT_TRUE(gm.joinGame(game2, 4, q4));

    auto games = gm.listGames();
    for (const auto &g : games)
        EXPECT_EQ(g.playerCount, 2);

    gm.stopAll();
}

TEST(GameManagerTest, RemoveClientDecreasesPlayerCount)
{
    GameManager gm;
    uint32_t gameId = gm.createGame("sala", 4);

    Queue<std::shared_ptr<const Message>> clientQueue;
    gm.joinGame(gameId, 1, clientQueue);
    gm.removeClient(1);

    auto games = gm.listGames();
    EXPECT_EQ(games[0].playerCount, 0);
    gm.stopAll();
}

TEST(GameManagerTest, RemoveNonExistentClientDoesNotCrash)
{
    GameManager gm;
    EXPECT_NO_THROW(gm.removeClient(999));
    gm.stopAll();
}

TEST(GameManagerTest, AfterRemoveClientCanRejoin)
{
    GameManager gm;
    uint32_t gameId = gm.createGame("sala", 1);

    Queue<std::shared_ptr<const Message>> q1, q2;
    EXPECT_TRUE(gm.joinGame(gameId, 1, q1));
    gm.removeClient(1);
    EXPECT_TRUE(gm.joinGame(gameId, 2, q2)); // slot liberado
    gm.stopAll();
}

TEST(GameManagerTest, ConcurrentJoinsRespectMaxPlayers)
{
    GameManager gm;
    uint32_t gameId = gm.createGame("sala", 5);

    constexpr int NUM_CLIENTS = 20;
    std::vector<Queue<std::shared_ptr<const Message>>> queues(NUM_CLIENTS);
    std::vector<bool> results(NUM_CLIENTS, false);
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_CLIENTS; ++i)
    {
        threads.emplace_back([&, i]()
                             { results[i] = gm.joinGame(gameId, i + 1, queues[i]); });
    }

    for (auto &t : threads)
        t.join();

    int accepted = 0;
    for (bool r : results)
        if (r)
            ++accepted;

    EXPECT_EQ(accepted, 5);
    gm.stopAll();
}

TEST(GameManagerTest, ConcurrentCreateAndList)
{
    GameManager gm;
    constexpr int NUM_ROOMS = 10;
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_ROOMS; ++i)
    {
        threads.emplace_back([&, i]()
                             { gm.createGame("sala" + std::to_string(i), 4); });
    }

    for (auto &t : threads)
        t.join();

    auto games = gm.listGames();
    EXPECT_EQ(games.size(), static_cast<size_t>(NUM_ROOMS));
    gm.stopAll();
}

TEST(GameManagerTest, ConcurrentJoinAndRemove)
{
    GameManager gm;
    uint32_t gameId = gm.createGame("sala", 10);

    constexpr int NUM_CLIENTS = 10;
    std::vector<Queue<std::shared_ptr<const Message>>> queues(NUM_CLIENTS);
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_CLIENTS; ++i)
        gm.joinGame(gameId, i + 1, queues[i]);

    for (int i = 0; i < NUM_CLIENTS; ++i)
    {
        threads.emplace_back([&, i]()
                             {
            gm.removeClient(i + 1);
            gm.joinGame(gameId, i + 1 + NUM_CLIENTS, queues[i]); });
    }

    for (auto &t : threads)
        t.join();

    EXPECT_NO_FATAL_FAILURE(gm.listGames());
    gm.stopAll();
}

TEST(ClientRegistryTest, AddAndGetQueue)
{
    ClientRegistry registry;
    Queue<std::shared_ptr<const Message>> q;

    registry.add(1, q);
    EXPECT_EQ(registry.get(1), &q);
}

TEST(ClientRegistryTest, GetNonExistentReturnsNullptr)
{
    ClientRegistry registry;
    EXPECT_EQ(registry.get(999), nullptr);
}

TEST(ClientRegistryTest, RemoveThenGetReturnsNullptr)
{
    ClientRegistry registry;
    Queue<std::shared_ptr<const Message>> q;

    registry.add(1, q);
    registry.remove(1);
    EXPECT_EQ(registry.get(1), nullptr);
}

TEST(ClientRegistryTest, ConcurrentAddAndGet)
{
    ClientRegistry registry;
    constexpr int N = 50;
    std::vector<Queue<std::shared_ptr<const Message>>> queues(N);
    std::vector<std::thread> threads;

    for (int i = 0; i < N; ++i)
    {
        threads.emplace_back([&, i]()
                             { registry.add(i, queues[i]); });
    }

    for (auto &t : threads)
        t.join();

    for (int i = 0; i < N; ++i)
        EXPECT_EQ(registry.get(i), &queues[i]);
}

TEST(ClientRegistryTest, ConcurrentAddAndRemove)
{
    ClientRegistry registry;
    constexpr int N = 50;
    std::vector<Queue<std::shared_ptr<const Message>>> queues(N);
    std::vector<std::thread> addThreads, removeThreads;

    for (int i = 0; i < N; ++i)
        registry.add(i, queues[i]);

    for (int i = 0; i < N; ++i)
    {
        removeThreads.emplace_back([&, i]()
                                   { registry.remove(i); });
    }

    for (auto &t : removeThreads)
        t.join();

    for (int i = 0; i < N; ++i)
        EXPECT_EQ(registry.get(i), nullptr);
}

TEST(MonitorTest, SendToDeliversMessage)
{
    Monitor monitor;
    Queue<std::shared_ptr<const Message>> q;
    monitor.addQueue(1, q);

    auto msg = std::make_shared<const ConnectOkMessage>();
    monitor.sendTo(1, msg);

    std::shared_ptr<const Message> received;
    EXPECT_TRUE(q.try_pop(received));
    EXPECT_EQ(received->opCode(), msg->opCode());
}

TEST(MonitorTest, SendToUnknownClientDoesNotCrash)
{
    Monitor monitor;
    auto msg = std::make_shared<const ConnectOkMessage>();
    EXPECT_NO_THROW(monitor.sendTo(999, msg));
}

TEST(MonitorTest, BroadcastDeliversToAll)
{
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

TEST(MonitorTest, RemoveQueueStopsSendTo)
{
    Monitor monitor;
    Queue<std::shared_ptr<const Message>> q;
    monitor.addQueue(1, q);
    monitor.removeQueue(1);

    auto msg = std::make_shared<const ConnectOkMessage>();
    monitor.sendTo(1, msg);

    std::shared_ptr<const Message> received;
    EXPECT_FALSE(q.try_pop(received));
}

TEST(MonitorTest, SizeReflectsAddAndRemove)
{
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

TEST(MonitorTest, ConcurrentSendTo)
{
    Monitor monitor;
    constexpr int N = 10;
    std::vector<Queue<std::shared_ptr<const Message>>> queues(N);

    for (int i = 0; i < N; ++i)
        monitor.addQueue(i, queues[i]);

    std::vector<std::thread> threads;
    for (int i = 0; i < N; ++i)
    {
        threads.emplace_back([&, i]()
                             {
            auto msg = std::make_shared<const ConnectOkMessage>();
            monitor.sendTo(i, msg); });
    }

    for (auto &t : threads)
        t.join();

    for (int i = 0; i < N; ++i)
    {
        std::shared_ptr<const Message> r;
        EXPECT_TRUE(queues[i].try_pop(r));
    }
}