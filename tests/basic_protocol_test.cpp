#include "gtest/gtest.h"

#include "common/network/protocol/packetWriter.h"
#include "common/network/protocol/packetReader.h"
#include "common/network/protocol/registry.h"
#include "common/network/messages/message.h"
#include "common/network/messages/client/auth/connectMessage.h"
#include "common/network/messages/server/auth/connectOKMessage.h"
#include "common/network/protocol/clientOpCode.h"
#include "common/network/protocol/serverOpCode.h"
#include "common/network/messages/server/player/EntityMoveMessage.h"

namespace
{

    TEST(PacketWriterTest, WriteAndReadUint8)
    {
        PacketWriter writer;
        writer.writeUint8(0xAB);

        PacketReader reader(writer.data(), writer.size());
        EXPECT_EQ(reader.readUint8(), 0xAB);
    }

    TEST(PacketWriterTest, WriteAndReadUint16)
    {
        PacketWriter writer;
        writer.writeUint16(0x1234);

        PacketReader reader(writer.data(), writer.size());
        EXPECT_EQ(reader.readUint16(), 0x1234);
    }

    TEST(PacketWriterTest, WriteAndReadUint32)
    {
        PacketWriter writer;
        writer.writeUint32(0xDEADBEEF);

        PacketReader reader(writer.data(), writer.size());
        EXPECT_EQ(reader.readUint32(), 0xDEADBEEF);
    }

    TEST(PacketWriterTest, WriteAndReadString)
    {
        PacketWriter writer;
        writer.writeString("hola_mundo");

        PacketReader reader(writer.data(), writer.size());
        EXPECT_EQ(reader.readString(), "hola_mundo");
    }

    TEST(PacketWriterTest, WriteAndReadEmptyString)
    {
        PacketWriter writer;
        writer.writeString("");

        PacketReader reader(writer.data(), writer.size());
        EXPECT_EQ(reader.readString(), "");
    }

    TEST(PacketWriterTest, WriteMultipleFieldsRoundTrip)
    {
        PacketWriter writer;
        writer.writeUint8(7);
        writer.writeUint16(1000);
        writer.writeString("argentum");
        writer.writeUint32(999999);

        PacketReader reader(writer.data(), writer.size());
        EXPECT_EQ(reader.readUint8(), 7);
        EXPECT_EQ(reader.readUint16(), 1000);
        EXPECT_EQ(reader.readString(), "argentum");
        EXPECT_EQ(reader.readUint32(), 999999U);
    }

    TEST(PacketWriterTest, ClearResetsBuffer)
    {
        PacketWriter writer;
        writer.writeUint32(0xFFFFFFFF);
        writer.clear();
        EXPECT_EQ(writer.size(), 0u);
    }

    TEST(PacketWriterTest, SizeMatchesWrittenBytes)
    {
        PacketWriter writer;
        writer.writeUint8(1);     // 1 byte
        writer.writeUint16(2);    // 2 bytes
        writer.writeUint32(3);    // 4 bytes
        writer.writeString("ab"); // 2 (length prefix) + 2 = 4 bytes
        EXPECT_EQ(writer.size(), 11u);
    }

    TEST(PacketReaderTest, OverflowThrows)
    {
        uint8_t buf[1] = {0xFF};
        PacketReader reader(buf, sizeof(buf));
        reader.readUint8(); // consume el único byte
        EXPECT_THROW(reader.readUint8(), std::runtime_error);
    }

    TEST(PacketReaderTest, HasMoreReturnsFalseWhenExhausted)
    {
        uint8_t buf[1] = {0x00};
        PacketReader reader(buf, sizeof(buf));
        EXPECT_TRUE(reader.hasMore());
        reader.readUint8();
        EXPECT_FALSE(reader.hasMore());
    }

    TEST(PacketReaderTest, RemainingDecreasesCorrectly)
    {
        PacketWriter writer;
        writer.writeUint8(1);
        writer.writeUint16(2);

        PacketReader reader(writer.data(), writer.size());
        EXPECT_EQ(reader.remaining(), 3u);
        reader.readUint8();
        EXPECT_EQ(reader.remaining(), 2u);
        reader.readUint16();
        EXPECT_EQ(reader.remaining(), 0u);
    }

    TEST(ConnectMessageTest, OpCode)
    {
        ConnectMessage msg(1, "jugador");
        EXPECT_EQ(msg.opCode(), static_cast<uint8_t>(ClientOpCode::MSG_CONNECT));
    }

    TEST(ConnectMessageTest, SerializeAndDeserializeBody)
    {
        const uint8_t version = 2;
        const std::string user = "pepe123";

        ConnectMessage msg(version, user);

        PacketWriter writer;
        msg.serializeBody(writer);

        PacketReader reader(writer.data(), writer.size());
        EXPECT_EQ(reader.readUint8(), version);
        EXPECT_EQ(reader.readString(), user);
        EXPECT_FALSE(reader.hasMore());
    }

    TEST(ConnectMessageTest, Getters)
    {
        ConnectMessage msg(3, "gandalf");
        EXPECT_EQ(msg.version(), 3);
        EXPECT_EQ(msg.getUsername(), "gandalf");
    }

    TEST(ConnectMessageTest, EmptyUsername)
    {
        ConnectMessage msg(1, "");

        PacketWriter writer;
        msg.serializeBody(writer);

        PacketReader reader(writer.data(), writer.size());
        reader.readUint8(); // version
        EXPECT_EQ(reader.readString(), "");
    }

    TEST(ConnectMessageTest, LongUsername)
    {
        std::string longName(500, 'x');
        ConnectMessage msg(1, longName);

        PacketWriter writer;
        msg.serializeBody(writer);

        PacketReader reader(writer.data(), writer.size());
        reader.readUint8();
        EXPECT_EQ(reader.readString(), longName);
    }

    TEST(ConnectOkMessageTest, OpCode)
    {
        ConnectOkMessage msg;
        EXPECT_EQ(msg.opCode(), static_cast<uint8_t>(ServerOpCode::MSG_CONNECT_OK));
    }

    TEST(ConnectOkMessageTest, BodyIsEmpty)
    {
        ConnectOkMessage msg;

        PacketWriter writer;
        msg.serializeBody(writer);

        EXPECT_EQ(writer.size(), 0u);
    }

    TEST(RegistryTest, RegisterAndDeserializeKnownOpcode)
    {
        Registry registry;

        registry.registerDeserializer(
            static_cast<uint8_t>(ServerOpCode::MSG_CONNECT_OK),
            [](PacketReader &) -> std::unique_ptr<Message>
            {
                return std::make_unique<ConnectOkMessage>();
            });

        // body vacio para MSG_CONNECT_OK
        PacketWriter writer;
        PacketReader reader(writer.data(), writer.size());

        auto msg = registry.deserialize(
            static_cast<uint8_t>(ServerOpCode::MSG_CONNECT_OK), reader);

        ASSERT_NE(msg, nullptr);
        EXPECT_EQ(msg->opCode(),
                  static_cast<uint8_t>(ServerOpCode::MSG_CONNECT_OK));
    }

    TEST(RegistryTest, UnknownOpcodeThrows)
    {
        Registry registry;

        PacketWriter writer;
        PacketReader reader(writer.data(), writer.size());

        EXPECT_THROW(
            registry.deserialize(0xFF, reader),
            std::runtime_error);
    }

    TEST(RegistryTest, RegisterConnectMessageDeserializer)
    {
        Registry registry;

        registry.registerDeserializer(
            static_cast<uint8_t>(ClientOpCode::MSG_CONNECT),
            [](PacketReader &reader) -> std::unique_ptr<Message>
            {
                auto version = reader.readUint8();
                auto username = reader.readString();
                return std::make_unique<ConnectMessage>(version, std::move(username));
            });

        // Serializar un ConnectMessage de prueba
        ConnectMessage original(5, "testUser");
        PacketWriter bodyWriter;
        original.serializeBody(bodyWriter);

        PacketReader reader(bodyWriter.data(), bodyWriter.size());
        auto msg = registry.deserialize(
            static_cast<uint8_t>(ClientOpCode::MSG_CONNECT), reader);

        ASSERT_NE(msg, nullptr);

        auto *connect = dynamic_cast<ConnectMessage *>(msg.get());
        ASSERT_NE(connect, nullptr);
        EXPECT_EQ(connect->version(), 5);
        EXPECT_EQ(connect->getUsername(), "testUser");
    }

    TEST(RegistryTest, OverwritingOpcodeUsesLastRegistered)
    {
        Registry registry;

        registry.registerDeserializer(
            0x01,
            [](PacketReader &) -> std::unique_ptr<Message>
            {
                return std::make_unique<ConnectOkMessage>();
            });

        registry.registerDeserializer(
            0x01,
            [](PacketReader &reader) -> std::unique_ptr<Message>
            {
                auto v = reader.readUint8();
                auto u = reader.readString();
                return std::make_unique<ConnectMessage>(v, std::move(u));
            });

        ConnectMessage probe(1, "x");
        PacketWriter writer;
        probe.serializeBody(writer);

        PacketReader reader(writer.data(), writer.size());
        auto msg = registry.deserialize(0x01, reader);

        EXPECT_NE(dynamic_cast<ConnectMessage *>(msg.get()), nullptr);
    }

    TEST(ProtocolIntegrationTest, ConnectMessageFrameRoundTrip)
    {
        ConnectMessage original(1, "hernan");

        PacketWriter bodyWriter;
        original.serializeBody(bodyWriter);

        PacketWriter packet;
        packet.writeUint8(original.opCode());
        packet.writeUint16(static_cast<uint16_t>(bodyWriter.size()));
        packet.writeBytes(bodyWriter.data(), bodyWriter.size());

        PacketReader frameReader(packet.data(), packet.size());

        const uint8_t opcode = frameReader.readUint8();
        const uint16_t bodyLength = frameReader.readUint16();

        EXPECT_EQ(opcode, static_cast<uint8_t>(ClientOpCode::MSG_CONNECT));
        EXPECT_EQ(bodyLength, bodyWriter.size());

        std::vector<uint8_t> body(bodyLength);
        for (size_t i = 0; i < bodyLength; ++i)
            body[i] = frameReader.readUint8();

        PacketReader bodyReader(body.data(), body.size());
        const uint8_t version = bodyReader.readUint8();
        const auto username = bodyReader.readString();

        EXPECT_EQ(version, 1);
        EXPECT_EQ(username, "hernan");
        EXPECT_FALSE(bodyReader.hasMore());
    }

    TEST(ProtocolIntegrationTest, ConnectOkMessageFrameRoundTrip)
    {
        ConnectOkMessage original;

        PacketWriter bodyWriter;
        original.serializeBody(bodyWriter);

        PacketWriter packet;
        packet.writeUint8(original.opCode());
        packet.writeUint16(static_cast<uint16_t>(bodyWriter.size()));

        PacketReader frameReader(packet.data(), packet.size());
        const uint8_t opcode = frameReader.readUint8();
        const uint16_t bodyLength = frameReader.readUint16();

        EXPECT_EQ(opcode, static_cast<uint8_t>(ServerOpCode::MSG_CONNECT_OK));
        EXPECT_EQ(bodyLength, 0u);
        EXPECT_FALSE(frameReader.hasMore());
    }


    TEST(EntityMoveMessageTest, OpCode) {
    EntityMoveMessage msg(1, 400, 320,Direction::DOWN,true);
    EXPECT_EQ(msg.opCode(), static_cast<uint8_t>(ServerOpCode::MSG_ENTITY_MOVE));
}

TEST(EntityMoveMessageTest, RoundTrip) {

    EntityMoveMessage original(1, 400, 320, Direction::DOWN, true);
    PacketWriter writer;
    original.serializeBody(writer);

    PacketReader reader(writer.data(), writer.size());
    auto id = reader.readUint8();
    auto x  = reader.readUint16();
    auto y  = reader.readUint16();

    EXPECT_EQ(id, 1);
    EXPECT_EQ(x, 400);
    EXPECT_EQ(y, 320);
    EXPECT_FALSE(reader.hasMore());
}

TEST(EntityMoveMessageTest, Getters) {
    EntityMoveMessage msg(3, 1500, 960, Direction::RIGHT, true);
    EXPECT_EQ(msg.getId(), 3);
    EXPECT_EQ(msg.getX(), 1500);
    EXPECT_EQ(msg.getY(), 960);
}

TEST(EntityMoveMessageTest, MaxMapCoords) {
    // Máximo del mapa: 25*96=2400, 20*96=1920
    EntityMoveMessage msg(1, 2400, 1920, Direction::DOWN, true);
    PacketWriter writer;
    msg.serializeBody(writer);
    PacketReader reader(writer.data(), writer.size());
    reader.readUint8();
    EXPECT_EQ(reader.readUint16(), 2400);
    EXPECT_EQ(reader.readUint16(), 1920);
}
}