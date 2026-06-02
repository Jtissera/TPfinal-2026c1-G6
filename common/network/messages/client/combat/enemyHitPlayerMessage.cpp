
#include "enemyHitPlayerMessage.h"
#include "common/network/protocol/clientOpCode.h"

EnemyHitPlayerMessage::EnemyHitPlayerMessage(uint32_t enemyId)
    : enemyId(enemyId) {}

uint8_t EnemyHitPlayerMessage::opCode() const {
    return static_cast<uint8_t>(ClientOpCode::MSG_ENEMY_HIT_PLAYER);
}

void EnemyHitPlayerMessage::serializeBody(PacketWriter& writer) const {
    writer.writeUint32(enemyId);
}

uint32_t EnemyHitPlayerMessage::getEnemyId() const {
    return enemyId;
}
