//
// Created by mauro on 26/5/26.
//

#include "AttackSystem.h"
#include <algorithm>
#include <iostream>


// #include "common/network/messages/client/combat/attackMessage.h"

void AttackSystem::handleMouseClick(
    int screenX,
    int screenY,
    const SDL_Rect& camera,
    std::map<uint32_t, Entity*>& enemies,
    Queue<std::shared_ptr<const Message>>* sendQueue,
    Entity* player,
    const ItemView* equippedWeapon
) {
    // Convertimos coordenadas de pantalla a coordenadas de mundo.
    // El -133 compensa el offset vertical del área del mapa.
    int worldX = screenX + camera.x;
    int worldY = screenY - 133 + camera.y;

    for (auto& [id, entity] : enemies) {
        if (entity == nullptr) {
            continue;
        }

        auto& tf = entity->getComponent<TransformComponent>();

        int enemyX = static_cast<int>(tf.position.x);
        int enemyY = static_cast<int>(tf.position.y);

        // Tamaño aproximado del enemigo visible.
        int enemyW = 64;
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
            entity->destroy();
            enemies.erase(id);
            enemyHealth.erase(id);
        }

        return;
    }
}
bool AttackSystem::applyDamage(uint32_t targetId, int damage) {
    // Si el enemigo todavía no tiene vida registrada, le damos vida inicial.
    // Para demo: skeleton con 100 de vida.
    if (enemyHealth.find(targetId) == enemyHealth.end()) {
        enemyHealth[targetId] = 100;
    }

    // Aplicamos daño.
    enemyHealth[targetId] -= damage;

    std::cout << "Enemigo id=" << targetId
              << " vida restante=" << enemyHealth[targetId]
              << std::endl;

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