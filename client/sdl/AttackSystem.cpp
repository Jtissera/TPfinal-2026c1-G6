#include "AttackSystem.h"
#include <algorithm>
#include <iostream>
#include <cmath>

#include "common/network/messages/client/combat/attackMessage.h"
#include "world/RemotePlayer.h"



void AttackSystem::handleMouseClick(
    int screenX,
    int screenY,
    const SDL_Rect& camera,
    const std::vector<AttackTarget>& targets,
    Queue<std::shared_ptr<const Message>>* sendQueue,
    Entity* player,
    const ItemView* equippedWeapon
) {
    for (const AttackTarget& target : targets) {
        if (target.entity == nullptr) {
            continue;
        }

        SDL_Rect clickableRect{};

        if (target.entity->hasComponent<SpriteComponent>()) {
            const auto& sprite = target.entity->getComponent<SpriteComponent>();

            // getDestRect() representa dónde se ve realmente el sprite en pantalla.
            clickableRect = sprite.getDestRect();

            // Margen para que no sea tan difícil clickear enemigos chicos
            // o sprites con offsets visuales.
            clickableRect.x -= 10;
            clickableRect.y -= 10;
            clickableRect.w += 20;
            clickableRect.h += 20;
        } else {
            auto& tf = target.entity->getComponent<TransformComponent>();

            // Fallback por si alguna entidad atacable no tiene SpriteComponent.
            clickableRect = SDL_Rect{
                static_cast<int>(tf.position.x - camera.x),
                static_cast<int>(tf.position.y - camera.y + 133),
                96,
                96
            };
        }

        const bool clickedTarget =
            screenX >= clickableRect.x &&
            screenX <= clickableRect.x + clickableRect.w &&
            screenY >= clickableRect.y &&
            screenY <= clickableRect.y + clickableRect.h;

        if (!clickedTarget) {
            continue;
        }

        const int attackRange = attackRangeForWeapon(equippedWeapon);

        // Validación visual solamente.
        // No cortamos el ataque porque el server es la autoridad real.
        if (!isTargetInRange(player, *target.entity, attackRange)) {
        }

        // El cliente solo manda intención de ataque.
        sendAttackMessage(target.id, sendQueue);

        // Efecto visual local para bastón/hechizo.
        if (shouldCreateVisualEffect(equippedWeapon)) {
            createLocalAttackEffect(target.id, *target.entity, camera);
        }

        return;
    }
}
bool AttackSystem::applyDamage(uint32_t targetId, int damage) {
    // Si el enemigo todavía no tiene vida registrada, le damos vida inicial.
    // Para demo: skeleton con 100 de vida.
    if (isEnemyDead(targetId)) {
        return true;
    }
    if (enemyHealth.find(targetId) == enemyHealth.end()) {
        //mock de vida del enemigo;
        enemyHealth[targetId] = 50;
        enemyMaxHealth[targetId] = 50;
    }

    // Aplicamos daño.
    enemyHealth[targetId] -= damage;



    if (enemyHealth[targetId] < 0) {
        enemyHealth[targetId] = 0;
    }
    // Devuelve true si murió.
    return enemyHealth[targetId] <= 0;
}

void AttackSystem::createLocalAttackEffect(
    uint32_t /*targetId*/,
    Entity& target,
    const SDL_Rect& camera
) {
    if (!target.hasComponent<SpriteComponent>()) {
        return;
    }

    const auto& sprite = target.getComponent<SpriteComponent>();
    const SDL_Rect& targetRect = sprite.getDestRect();

    constexpr int effectSize = 64;

    const int screenCenterX = targetRect.x + targetRect.w / 2;
    const int screenCenterY = targetRect.y + targetRect.h / 2;

    AttackEffect effect{};

    effect.x = screenCenterX + camera.x - effectSize / 2;
    effect.y = screenCenterY + camera.y - 133 - effectSize / 2;

    effect.createdAt = SDL_GetTicks();
    effect.durationMs = 500;

    attackEffects.push_back(effect);
}

void AttackSystem::sendAttackMessage(uint32_t targetId,Queue<std::shared_ptr<const Message>>* sendQueue) {
    if (sendQueue == nullptr) {
        return;
    }

    sendQueue->try_push(std::make_shared<const AttackMessage>(targetId));
}

void AttackSystem::render(
    SDL_Renderer* renderer,
    AssetManager& assets,
    const SDL_Rect& camera
) {
    SDL_Texture* texAtk = assets.GetTexture("effect_attack_magic_01");

    if (texAtk == nullptr) {
        return;
    }

    Uint32 now = SDL_GetTicks();

    const int frameWidth = 64;
    const int frameHeight = 64;
    const int totalFrames = 11;

    for (auto& ef : attackEffects) {
        Uint32 elapsed = now - ef.createdAt;

        int frame = static_cast<int>((elapsed * totalFrames) / ef.durationMs);

        if (frame >= totalFrames) {
            frame = totalFrames - 1;
        }

        int srcX = 0;
        int srcY = 0;

        if (frame < 5) {
            srcX = frame * frameWidth;
            srcY = 0;
        } else {
            const int secondRowFrame = frame - 5;
            srcX = secondRowFrame * frameWidth;
            srcY = 64;
        }

        SDL_Rect src = { srcX, srcY, frameWidth, frameHeight };
        SDL_Rect dst = {
            ef.x - camera.x,
            ef.y - camera.y + 133,
            64,
            64
        };

        SDL_RenderCopy(renderer, texAtk, &src, &dst);
    }
}

int AttackSystem::attackRangeForWeapon(const ItemView* weapon) const {
    // Si no tiene arma, rango mínimo.
    if (weapon == nullptr) {
        return 35;
    }

    if (weapon->type == ClientItemType::MeleeWeapon) {
        // Espada, daga, hacha, etc.
        return 55;
    }

    if (weapon->type == ClientItemType::RangedWeapon) {
        // Arco. No hay flecha visible por ahora.
        return 220;
    }

    if (weapon->type == ClientItemType::MagicWeapon) {
        // Bastones. El efecto visual aparece sobre el objetivo.
        return 180;
    }

    return 35;
}

int AttackSystem::damageForWeapon(const ItemView* weapon) const {
    // Si no tiene arma, daño básico.
    if (weapon == nullptr) {
        return 5;
    }

    // Promedio simple entre daño mínimo y máximo.
    // Evitamos random por ahora para que sea más fácil testear.
    int damage = (weapon->damageMin + weapon->damageMax) / 2;

    if (damage <= 0) {
        damage = 1;
    }

    return damage;
}

bool AttackSystem::isTargetInRange(
    Entity* attacker,
    Entity& target,
    int range
) const {
    if (attacker == nullptr) {
        return false;
    }

    auto& attackerTf = attacker->getComponent<TransformComponent>();
    auto& targetTf = target.getComponent<TransformComponent>();

    // Centro aproximado del jugador.
    float attackerCenterX = attackerTf.position.x + 32.0f;
    float attackerCenterY = attackerTf.position.y + 64.0f;

    // Centro aproximado del enemigo.
    float targetCenterX = targetTf.position.x + 32.0f;
    float targetCenterY = targetTf.position.y + 64.0f;

    float dx = targetCenterX - attackerCenterX;
    float dy = targetCenterY - attackerCenterY;

    float distance = std::sqrt(dx * dx + dy * dy);

    return distance <= static_cast<float>(range);
}

bool AttackSystem::shouldCreateVisualEffect(const ItemView* weapon) const {
    if (weapon == nullptr) {
        return false;
    }

    // - espada: solo sonido/daño, sin efecto visual
    // - arco: solo sonido/daño, sin proyectil ni efecto
    // - bastón: efecto visual sobre el enemigo
    return weapon->type == ClientItemType::MagicWeapon;
}
void AttackSystem::markEnemyAsDead(uint32_t enemyId) {
    deadEnemies.insert(enemyId);
    enemyDeadAt[enemyId] = SDL_GetTicks();

    enemyHealth[enemyId] = 0;

    // Si murió, deja de perseguir.
    chasingEnemies.erase(enemyId);

    if (enemyMaxHealth.find(enemyId) == enemyMaxHealth.end()) {
        enemyMaxHealth[enemyId] = 100;
    }

}

bool AttackSystem::isEnemyDead(uint32_t enemyId) const {
    return deadEnemies.find(enemyId) != deadEnemies.end();
}

void AttackSystem::updateRespawns(std::map<uint32_t, Entity*>& enemies) {
    Uint32 now = SDL_GetTicks();

    std::vector<uint32_t> toRespawn;

    for (uint32_t enemyId : deadEnemies) {
        auto it = enemyDeadAt.find(enemyId);

        if (it == enemyDeadAt.end()) {
            continue;
        }

        Uint32 deadAt = it->second;

        if (now - deadAt >= enemyRespawnMs) {
            toRespawn.push_back(enemyId);
        }
    }

    for (uint32_t enemyId : toRespawn) {
        int maxHp = getEnemyMaxHealth(enemyId);

        enemyHealth[enemyId] = maxHp;

        auto enemyIt = enemies.find(enemyId);
        auto spawnIt = enemySpawnPositions.find(enemyId);

        if (enemyIt != enemies.end() &&
            enemyIt->second != nullptr &&
            spawnIt != enemySpawnPositions.end()) {

            auto& transform = enemyIt->second->getComponent<TransformComponent>();

            transform.position.x = spawnIt->second.x;
            transform.position.y = spawnIt->second.y;
        }

        deadEnemies.erase(enemyId);
        enemyDeadAt.erase(enemyId);
        chasingEnemies.erase(enemyId);

        enemyLastAttackAt.erase(enemyId);
    }
}

EnemyChaseResult AttackSystem::updateEnemyChase(std::map<uint32_t, Entity*>& enemies,Entity* player,int& playerHp) {
    if (player == nullptr) {
        return EnemyChaseResult::PlayerStillAlive;
    }

    if (playerHp <= 0) {
        playerHp = 0;
        clearEnemyAggro();
        return EnemyChaseResult::PlayerDied;
    }

    auto& playerTransform = player->getComponent<TransformComponent>();
    float playerCenterX = playerTransform.position.x + 16.0f;
    float playerCenterY = playerTransform.position.y + 32.0f;

    // PERF: iterar sin copiar el set — acumular borrados y aplicarlos al final.
    std::vector<uint32_t> toErase;

    for (uint32_t enemyId : chasingEnemies) {
        if (playerHp <= 0) {
            playerHp = 0;
            clearEnemyAggro();
            return EnemyChaseResult::PlayerDied;
        }

        if (isEnemyDead(enemyId)) {
            toErase.push_back(enemyId);
            enemyLastAttackAt.erase(enemyId);
            continue;
        }

        auto it = enemies.find(enemyId);
        if (it == enemies.end() || it->second == nullptr) {
            toErase.push_back(enemyId);
            enemyLastAttackAt.erase(enemyId);
            continue;
        }

        Entity* enemy = it->second;
        auto& enemyTransform = enemy->getComponent<TransformComponent>();

        float dx = playerCenterX - (enemyTransform.position.x + 16.0f);
        float dy = playerCenterY - (enemyTransform.position.y + 32.0f);

        // PERF: evitar sqrt — usar distSq para el check de posición mínima.
        float distSq = dx * dx + dy * dy;
        if (distSq <= 0.0001f)
            continue;

        // Normalizar: un sqrt por enemigo activo (inevitable para la dirección).
        float invDist = 1.0f / std::sqrt(distSq);
        enemyTransform.position.x += dx * invDist * enemyChaseSpeed;
        enemyTransform.position.y += dy * invDist * enemyChaseSpeed;
    }

    for (uint32_t id : toErase)
        chasingEnemies.erase(id);

    return EnemyChaseResult::PlayerStillAlive;
}

int AttackSystem::getEnemyHealth(uint32_t enemyId) const {
    auto it = enemyHealth.find(enemyId);

    if (it == enemyHealth.end()) {
        return 100;
    }

    return it->second;
}

int AttackSystem::getEnemyMaxHealth(uint32_t enemyId) const {
    auto it = enemyMaxHealth.find(enemyId);

    if (it == enemyMaxHealth.end()) {
        return 100;
    }

    return it->second;
}

void AttackSystem::clearEnemyAggro() {
    chasingEnemies.clear();
    enemyLastAttackAt.clear();
}

void AttackSystem::update() {
    Uint32 now = SDL_GetTicks();
    attackEffects.erase(
        std::remove_if(
            attackEffects.begin(),
            attackEffects.end(),
            [now](const AttackEffect& e) {
                return now - e.createdAt > e.durationMs;
            }
        ),
        attackEffects.end()
    );
}

void AttackSystem::setEnemyHealth(uint32_t enemyId, int hp, int maxHp) {
    if (maxHp < 0) maxHp = 0;
    if (hp < 0)    hp = 0;
    if (hp > maxHp) hp = maxHp;

    enemyHealth[enemyId]    = hp;
    enemyMaxHealth[enemyId] = maxHp;

    if (hp <= 0) {
        deadEnemies.insert(enemyId);
    } else {
        deadEnemies.erase(enemyId);
    }
}