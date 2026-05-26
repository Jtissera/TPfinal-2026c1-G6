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
    const std::map<uint32_t, Entity*>& enemies,
    Queue<std::shared_ptr<const Message>>* sendQueue
) {
    // Convertimos coordenadas de pantalla a coordenadas de mundo.
    // El +133 compensa que el mapa empieza debajo del HUD superior/chat.
    int worldX = screenX + camera.x;
    int worldY = screenY - 133 + camera.y;

    // Recorremos enemigos registrados.
    for (auto& [id, entity] : enemies) {
        if (entity == nullptr) {
            continue;
        }

        auto& tf = entity->getComponent<TransformComponent>();

        int enemyX = static_cast<int>(tf.position.x);
        int enemyY = static_cast<int>(tf.position.y);

        // Skeleton actual: frame 32x64 escalado x2.
        // Más adelante esto debería venir del collider o del sprite config.
        int enemyW = 64;
        int enemyH = 128;

        bool clickedEnemy =
            worldX >= enemyX &&
            worldX <= enemyX + enemyW &&
            worldY >= enemyY &&
            worldY <= enemyY + enemyH;

        if (clickedEnemy) {
            // Primero creamos efecto local para feedback visual inmediato.
            createLocalAttackEffect(id, *entity);

            // Por ahora NO mandamos al servidor para evitar caída de socket.
            // Cuando el protocolo esté listo, se descomenta dentro de sendAttackMessage().
            sendAttackMessage(id, sendQueue);

            return;
        }
    }
}

void AttackSystem::createLocalAttackEffect(uint32_t targetId, Entity& target) {
    auto& tf = target.getComponent<TransformComponent>();

    // Guardamos coordenadas de mundo.
    // Le sumamos un offset para que el efecto no quede en la esquina superior.
    int worldX = static_cast<int>(tf.position.x);
    int worldY = static_cast<int>(tf.position.y) + 20;

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
    // Mantener desactivado hasta que el servidor/protocolo soporte AttackMessage.
    // Si al activar esto se cierra el socket, el problema está en red/protocolo,
    // no en SDL ni en el render del efecto.

    (void)targetId;
    (void)sendQueue;

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
