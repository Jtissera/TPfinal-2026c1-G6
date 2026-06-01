//
// Created by mauro on 26/5/26.
//

#include "AttackSystem.h"
#include <algorithm>
#include <iostream>
#include <cmath>

// #include "common/network/messages/client/combat/attackMessage.h"

void AttackSystem::handleMouseClick(int screenX,int screenY,const SDL_Rect& camera,std::map<uint32_t, Entity*>& enemies,Queue<std::shared_ptr<const Message>>* sendQueue,Entity* player,const ItemView* equippedWeapon) {
    // Convertimos coordenadas de pantalla a coordenadas de mundo.
    // El -133 compensa el offset vertical del área del mapa.
    int worldX = screenX + camera.x;
    int worldY = screenY - 133 + camera.y;

    for (auto& [id, entity] : enemies) {
        if (entity == nullptr) {
            continue;
        }

        if (isEnemyDead(id)) {
            continue;
        }

        auto& tf = entity->getComponent<TransformComponent>();

        if (enemySpawnPositions.find(id) == enemySpawnPositions.end()) {
            enemySpawnPositions[id] = tf.position;
        }

        int enemyX = static_cast<int>(tf.position.x);
        int enemyY = static_cast<int>(tf.position.y);

        // Tamaño aproximado del enemigo visible.
        int enemyW = 128;
        int enemyH = 128;

        bool clickedEnemy =
            worldX >= enemyX &&
            worldX <= enemyX + enemyW &&
            worldY >= enemyY &&
            worldY <= enemyY + enemyH;

        if (!clickedEnemy) {
            continue;
        }

        // Obtenemos el rango según arma equipada.
        int attackRange = attackRangeForWeapon(equippedWeapon);

        // Si está fuera de rango, no se aplica daño.
        if (!isTargetInRange(player, *entity, attackRange)) {
            std::cout << "[ATTACK] Objetivo fuera de rango. Rango="
                      << attackRange
                      << std::endl;
            return;
        }

        // Por ahora está comentado dentro de sendAttackMessage.
        sendAttackMessage(id, sendQueue);

        // Calculamos daño según arma equipada.
        int damage = damageForWeapon(equippedWeapon);

        bool isDead = applyDamage(id, damage);

        // Solo mostramos efecto visual si el arma corresponde.
        if (shouldCreateVisualEffect(equippedWeapon)) {
            createLocalAttackEffect(id, *entity);
        }

        if (isDead) {
            markEnemyAsDead(id);
        }else {
            // Si recibió daño y sigue vivo, empieza a perseguir al jugador.
            chasingEnemies.insert(id);
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

void AttackSystem::createLocalAttackEffect(uint32_t targetId, Entity& target) {
    // Obtenemos la posición del objetivo.
    auto& tf = target.getComponent<TransformComponent>();

    // Guardamos coordenadas de mundo.
    // Sumamos offset para que el efecto no quede en la esquina superior.
    int worldX = static_cast<int>(tf.position.x);
    int worldY = static_cast<int>(tf.position.y) + 20;

    // Creamos el efecto visual local.
    attackEffects.push_back({
        worldX,
        worldY,
        SDL_GetTicks(),
        500
    });

    std::cout << "Ataque local sobre enemigo id=" << targetId << std::endl;
}

void AttackSystem::sendAttackMessage(
    uint32_t targetId,
    Queue<std::shared_ptr<const Message>>* sendQueue
) {
    // Evitamos warnings de variables no usadas mientras el socket está desactivado.
    (void)targetId;
    (void)sendQueue;

    // Cuando el protocolo de ataque esté listo, se reactiva esto:
    // auto msg = std::make_shared<AttackMessage>(targetId);
    // sendQueue->push(msg);
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

    for (auto& ef : attackEffects) {
        Uint32 elapsed = now - ef.createdAt;

        int totalFrames = 5;
        int frameWidth = 64;
        int frameHeight = 64;

        // Calcula el frame según el tiempo transcurrido.
        int frame = static_cast<int>((elapsed * totalFrames) / ef.durationMs);

        // Evita pasarse del último frame.
        if (frame >= totalFrames) {
            frame = totalFrames - 1;
        }

        // Recorte del spritesheet de ataque mágico.
        SDL_Rect src = {
            frame * frameWidth,
            64,
            frameWidth,
            frameHeight
        };

        // Convertimos mundo a pantalla restando cámara.
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

EnemyChaseResult AttackSystem::updateEnemyChase(
    std::map<uint32_t, Entity*>& enemies,
    Entity* player,
    int& playerHp
) {
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

    Uint32 now = SDL_GetTicks();

    // Copiamos los IDs porque clearEnemyAggro podría modificar el set
    // si el jugador muere durante el loop.
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
        if (distance <= enemyStopDistance) {
            Uint32 lastAttack = 0;

            auto lastIt = enemyLastAttackAt.find(enemyId);
            if (lastIt != enemyLastAttackAt.end()) {
                lastAttack = lastIt->second;
            }

            if (now - lastAttack >= enemyAttackCooldownMs) {
                // Aplicamos daño.
                playerHp -= enemyAttackDamage;

                // La vida nunca debe quedar negativa.
                if (playerHp < 0) {
                    playerHp = 0;
                }

                enemyLastAttackAt[enemyId] = now;

                std::cout << "[ENEMY ATTACK] enemigo id="
                          << enemyId
                          << " golpeó al jugador. HP jugador="
                          << playerHp
                          << std::endl;

                // Si este golpe mató al jugador, cortamos toda persecución.
                if (playerHp <= 0) {
                    clearEnemyAggro();

                    std::cout << "[PLAYER] El jugador murió por ataque enemigo."
                              << std::endl;

                    return EnemyChaseResult::PlayerDied;
                }
            }

            continue;
        }

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