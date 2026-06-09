
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
            std::cout << "[ATTACK] Cliente detecta fuera de rango. "
                      << "Se manda igual; server valida. Rango="
                      << attackRange
                      << std::endl;
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

    std::cout << "Enemigo id=" << targetId
              << " vida restante=" << enemyHealth[targetId]
              << std::endl;


    if (enemyHealth[targetId] < 0) {
        enemyHealth[targetId] = 0;
    }
    // Devuelve true si murió.
    return enemyHealth[targetId] <= 0;
}

void AttackSystem::createLocalAttackEffect(
    uint32_t targetId,
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

    std::cout << "[ATTACK EFFECT CREATE] targetId="
              << targetId
              << " world=("
              << effect.x
              << ", "
              << effect.y
              << ") total="
              << attackEffects.size()
              << std::endl;
}

void AttackSystem::sendAttackMessage(uint32_t targetId,Queue<std::shared_ptr<const Message>>* sendQueue) {
    // Si no hay cola de envío, no podemos mandar nada al server.
    if (sendQueue == nullptr) {
        return;
    }


    sendQueue->try_push(std::make_shared<const AttackMessage>(targetId));

    std::cout << "[ATTACK] AttackMessage enviado. targetId="<< targetId<< std::endl;
}

void AttackSystem::update() {
    Uint32 now = SDL_GetTicks();

    // Eliminamos los efectos que ya superaron su duración.
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

void AttackSystem::render(
    SDL_Renderer* renderer,
    AssetManager& assets,
    const SDL_Rect& camera
) {
    SDL_Texture* texAtk = assets.GetTexture("effect_attack_magic_01");

    if (texAtk == nullptr) {
        std::cout << "[ATTACK EFFECT] No se encontró textura: "
                  << "effect_attack_magic_01"
                  << std::endl;
        return;
    }

    Uint32 now = SDL_GetTicks();

    const int frameWidth = 64;
    const int frameHeight = 64;

    // Frames visibles del spritesheet.
    // Fila 0: primeros 5 frames.
    // Fila 1: primeros 6 frames.
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
            // Primera fila: columnas 0 a 4.
            srcX = frame * frameWidth;
            srcY = 0;
        } else {
            // Segunda fila: columnas 0 a 5.
            const int secondRowFrame = frame - 5;
            srcX = secondRowFrame * frameWidth;
            srcY = 64;
        }

        SDL_Rect src = {
            srcX,
            srcY,
            frameWidth,
            frameHeight
        };

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

    std::cout << "[ENEMY] enemigo id="
              << enemyId
              << " muerto visualmente. Respawn en "
              << enemyRespawnMs
              << " ms."
              << std::endl;
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
        std::cout << "[ENEMY] enemigo id="
                  << enemyId
                  << " reapareció con HP="
                  << maxHp
                  << std::endl;
    }
}

EnemyChaseResult AttackSystem::updateEnemyChase(std::map<uint32_t, Entity*>& enemies,Entity* player,int& playerHp) {
    // Si no hay jugador, no hay nada que perseguir.
    if (player == nullptr) {
        return EnemyChaseResult::PlayerStillAlive;
    }

    // Si el jugador ya está muerto, cortamos aggro y no actualizamos enemigos.
    // Esta regla evita que queden persiguiendo a un fantasma.
    if (playerHp <= 0) {
        playerHp = 0;
        clearEnemyAggro();
        return EnemyChaseResult::PlayerDied;
    }

    auto& playerTransform = player->getComponent<TransformComponent>();

    float playerCenterX = playerTransform.position.x + 16.0f;
    float playerCenterY = playerTransform.position.y + 32.0f;


    std::vector<uint32_t> chasingIds(
        chasingEnemies.begin(),
        chasingEnemies.end()
    );

    for (uint32_t enemyId : chasingIds) {
        // Si el jugador murió por un enemigo anterior en este mismo frame,
        // cortamos inmediatamente.
        if (playerHp <= 0) {
            playerHp = 0;
            clearEnemyAggro();
            return EnemyChaseResult::PlayerDied;
        }

        // Si el enemigo está muerto, no puede perseguir ni atacar.
        if (isEnemyDead(enemyId)) {
            chasingEnemies.erase(enemyId);
            enemyLastAttackAt.erase(enemyId);
            continue;
        }

        auto it = enemies.find(enemyId);

        // Si el enemigo no existe visualmente, limpiamos su estado.
        if (it == enemies.end() || it->second == nullptr) {
            chasingEnemies.erase(enemyId);
            enemyLastAttackAt.erase(enemyId);
            continue;
        }

        Entity* enemy = it->second;
        auto& enemyTransform = enemy->getComponent<TransformComponent>();

        float enemyCenterX = enemyTransform.position.x + 16.0f;
        float enemyCenterY = enemyTransform.position.y + 32.0f;

        float dx = playerCenterX - enemyCenterX;
        float dy = playerCenterY - enemyCenterY;

        float distance = std::sqrt(dx * dx + dy * dy);

        // Si están en la misma posición, no normalizamos para evitar división por cero.
        if (distance <= 0.01f) {
            continue;
        }

        // Si el enemigo está cerca, intenta atacar con cooldown.
        // if (distance <= enemyStopDistance) {
        //     Uint32 lastAttack = 0;
        //
        //     auto lastIt = enemyLastAttackAt.find(enemyId);
        //     if (lastIt != enemyLastAttackAt.end()) {
        //         lastAttack = lastIt->second;
        //     }
        //
        //     // if (now - lastAttack >= enemyAttackCooldownMs) {
        //     //     enemyLastAttackAt[enemyId] = now;
        //     //     if (sendQueue != nullptr) {
        //     //         sendQueue->try_push(std::make_shared<const EnemyHitPlayerMessage>(enemyId));
        //     //     }
        //     //
        //     //     std::cout << "[ENEMY ATTACK] enemigo id="
        //     //               << enemyId
        //     //               << " atacó. Mensaje enviado al server."
        //     //               << std::endl;
        //     //
        //     // }
        //
        //     continue;
        // }

        // Si está lejos, persigue.
        float dirX = dx / distance;
        float dirY = dy / distance;

        enemyTransform.position.x += dirX * enemyChaseSpeed;
        enemyTransform.position.y += dirY * enemyChaseSpeed;
    }

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
    // Limpiamos todos los enemigos que estaban persiguiendo al jugador.
    // Esto corta el estado de combate cuando el jugador muere.
    chasingEnemies.clear();

    // Limpiamos cooldowns de ataque para no conservar estado viejo.
    // Si luego el jugador revive, los enemigos no deben pegar instantáneamente
    // por cooldown heredado de una vida anterior.
    enemyLastAttackAt.clear();

    std::cout << "[ENEMY] Aggro limpiado. Los enemigos dejan de perseguir." << std::endl;
}

void AttackSystem::setEnemyHealth(uint32_t enemyId, int hp, int maxHp) {
    // Normalizamos valores para evitar vida negativa o máximo inválido.
    if (maxHp < 0) {
        maxHp = 0;
    }

    if (hp < 0) {
        hp = 0;
    }

    if (hp > maxHp) {
        hp = maxHp;
    }

    enemyHealth[enemyId] = hp;
    enemyMaxHealth[enemyId] = maxHp;

    if (hp <= 0) {
        deadEnemies.insert(enemyId);
    } else {
        deadEnemies.erase(enemyId);
    }

    std::cout << "[ATTACK SYSTEM] vida NPC actualizada id="
              << enemyId
              << " hp="
              << hp
              << "/"
              << maxHp
              << std::endl;
}