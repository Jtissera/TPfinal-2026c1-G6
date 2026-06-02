#include "npcManager.h"
#include <cstdlib>
#include <algorithm>
#include <cmath>

NpcManager::NpcManager(NpcFactory& factory,
                        const CollisionSystem& collision,
                        const MapData& mapData,
                        int tileSize)
    : factory(factory)
    , mapData(mapData)
    , collision(collision)
    , tileSize(tileSize)
{}

uint32_t NpcManager::spawnNpc(const std::string& typeName,
                               float pixelX, float pixelY) {
    static uint32_t nextId = 1000;
    uint32_t id = nextId++;
    auto npc = factory.create(id, typeName, pixelX, pixelY, tileSize);
    npcs.emplace(id, std::move(npc));
    return id;
}

void NpcManager::applyMove(uint32_t npcId, float toX, float toY) {
    auto it = npcs.find(npcId);
    if (it != npcs.end())
        it->second.setPixelPos(toX, toY);
}

bool NpcManager::isSameZone(float pixelX, float pixelY,
                              ZoneType zone) const {
    int tx = static_cast<int>(pixelX) / tileSize;
    int ty = static_cast<int>(pixelY) / tileSize;
    if (!collision.isInBoundsTile(tx, ty)) return false;
    return mapData.at(tx, ty).zone == zone;
}

NpcTickResult NpcManager::tick(
    const std::unordered_map<uint32_t, Player>& players)
{
    NpcTickResult result;

    std::vector<uint32_t> toRemove;

    for (auto& [id, npc] : npcs) {
        if (!npc.isAlive()) {
            toRemove.push_back(id);
            result.deaths.push_back(buildDeathResult(npc));
            continue;
        }

        // --- Deteccion de jugadores ---
        // Busca el jugador vivo mas cercano dentro del rango de deteccion
        uint32_t nearestId = 0;
        float    nearestDist = npc.getDetectionRangePx() + 1.0f;

        for (const auto& [pid, player] : players) {
            if (!player.isAlive()) continue;

            float dx = std::abs(npc.getPixelX() - player.getPixelX());
            float dy = std::abs(npc.getPixelY() - player.getPixelY());
            float dist = std::max(dx, dy);   // Chebyshev

            if (dist < nearestDist) {
                nearestDist = dist;
                nearestId   = pid;
            }
        }

        // --- Maquina de estados ---
        if (nearestId != 0) {
            npc.setState(NpcState::CHASING);
            npc.setTargetId(nearestId);
        } else if (npc.getState() == NpcState::CHASING) {
            // Perdio al objetivo
            npc.clearTarget();
        }

        if (npc.getState() == NpcState::CHASING) {
            const Player& target = players.at(npc.getTargetId());

            float dx = std::abs(npc.getPixelX() - target.getPixelX());
            float dy = std::abs(npc.getPixelY() - target.getPixelY());
            float dist = std::max(dx, dy);

            // --- Ataque ---
            if (dist <= npc.getAttackRangePx() && npc.canAttack()) {
                int16_t dmg = rollDamage(npc.getStats());
                result.attacks.push_back({id, npc.getTargetId(), dmg});
                npc.resetAttackCooldown();
            }
            // --- Movimiento de persecucion ---
            else if (npc.canMove()) {
                // Determina direccion hacia el target (un tile por vez)
                float step = npc.getStepPx();
                float nx   = npc.getPixelX();
                float ny   = npc.getPixelY();

                float rawDx = target.getPixelX() - npc.getPixelX();
                float rawDy = target.getPixelY() - npc.getPixelY();

                if (std::abs(rawDx) >= std::abs(rawDy)) {
                    nx += (rawDx > 0) ? step : -step;
                } else {
                    ny += (rawDy > 0) ? step : -step;
                }

                // Verifica home range: no perseguir mas alla de homeRangePx
                float hdx = std::abs(nx - npc.getSpawnPixelX());
                float hdy = std::abs(ny - npc.getSpawnPixelY());
                if (std::max(hdx, hdy) <= npc.getHomeRangePx()) {
                    result.moveIntents.push_back({
                        id,
                        npc.getPixelX(), npc.getPixelY(),
                        nx, ny
                    });
                }

                npc.resetMoveCooldown();
            }
        }
        // --- Patrulla idle (camina random cerca del spawn) ---
        else if (npc.getState() == NpcState::IDLE && npc.canMove()) {
            static const float dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
            int d = std::rand() % 4;
            float step = npc.getStepPx();
            float nx = npc.getPixelX() + dirs[d][0] * step;
            float ny = npc.getPixelY() + dirs[d][1] * step;

            float hdx = std::abs(nx - npc.getSpawnPixelX());
            float hdy = std::abs(ny - npc.getSpawnPixelY());
            if (std::max(hdx, hdy) <= npc.getHomeRangePx()) {
                result.moveIntents.push_back({
                    id,
                    npc.getPixelX(), npc.getPixelY(),
                    nx, ny
                });
            }

            npc.resetMoveCooldown();
        }
    }

    for (uint32_t id : toRemove)
        npcs.erase(id);

    return result;
}

int16_t NpcManager::rollDamage(const NpcStats& stats) const {
    int range = stats.damageMax - stats.damageMin;
    return static_cast<int16_t>(
        stats.damageMin + (range > 0 ? std::rand() % range : 0));
}

NpcDeathResult NpcManager::buildDeathResult(const Npc& npc) const {
    NpcDeathResult r;
    r.npcId  = npc.getId();
    r.pixelX = npc.getPixelX();
    r.pixelY = npc.getPixelY();
    r.tileX  = npc.getTileX();
    r.tileY  = npc.getTileY();

    // Drop de oro: chance segun stats
    float goldChance = static_cast<float>(std::rand() % 100) / 100.0f;
    if (goldChance < npc.getStats().goldDropChance) {
        int range = npc.getStats().goldDropMax - npc.getStats().goldDropMin;
        r.goldDrop = npc.getStats().goldDropMin +
                     (range > 0 ? std::rand() % range : 0);
    }

    // Drop de item: chance segun stats
    float itemChance = static_cast<float>(std::rand() % 100) / 100.0f;
    if (itemChance < npc.getStats().itemDropChance)
        r.itemDrop = npc.getStats().itemDrop;

    return r;
}