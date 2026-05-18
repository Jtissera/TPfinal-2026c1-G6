
void GameServerDeserializersModule::registerDeserializers(
    Registry &registry) const {
  registry.registerDeserializer(
      static_cast<uint8_t>(ServerOpCode::MSG_ENTITY_MOVE),
      [](PacketReader &reader) -> std::unique_ptr<Message> {
        auto entityId = reader.readUint32();
        auto x = reader.readUint16();
        auto y = reader.readUint16();
        return std::make_unique<EntityMoveMessage>(entityId, x, y);
      });
}