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
    Queue<std::shared_ptr<const Message>>* sendQueue
) {
    // Convertimos coordenadas de pantalla a coordenadas de mundo.
    // El -133 compensa el offset vertical del HUD/mapa.
    int worldX = screenX + camera.x;
    int worldY = screenY - 133 + camera.y;

    // Recorremos enemigos registrados.
    for (auto& [id, entity] : enemies) {
        if (entity == nullptr) {
            continue;
        }

        // Obtenemos la posición del enemigo.
        auto& tf = entity->getComponent<TransformComponent>();

        int enemyX = static_cast<int>(tf.position.x);
        int enemyY = static_cast<int>(tf.position.y);

        // Tamaño visual aproximado del skeleton:
        // frame 32x64 escalado x2 => 64x128.
        int enemyW = 64;
        int enemyH = 128;

        // Verificamos si el click cayó dentro del rectángulo del enemigo.
        bool clickedEnemy =
            worldX >= enemyX &&
            worldX <= enemyX + enemyW &&
            worldY >= enemyY &&
            worldY <= enemyY + enemyH;

        if (clickedEnemy) {
            //efecto visual de ataque.
            createLocalAttackEffect(id, *entity);

            // Por ahora está comentado dentro de sendAttackMessage.
            sendAttackMessage(id, sendQueue);

            // Para demo: cada ataque hace 25 de daño.
            bool isDead = applyDamage(id, 25);

            if (isDead) {
                entity->destroy();
                enemies.erase(id);
                enemyHealth.erase(id);
            }

            return;
        }
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
    SDL_Texture* texAtk = assets.GetTexture("ataque1");

    if (texAtk == nullptr) {
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
