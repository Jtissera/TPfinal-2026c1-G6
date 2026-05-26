//
// Created by mauro on 26/5/26.
//

#ifndef TALLER_TP_ATTACKSYSTEM_H
#define TALLER_TP_ATTACKSYSTEM_H
#include <SDL_render.h>
#include "ECS/Components.h"
#include "AssetManager.h"
#include <SDL2/SDL_rect.h>
#include <unordered_map>

// Representa un efecto visual de ataque activo.
// Por ahora solo guarda posición, tiempo de creación y duración.
struct AttackEffect {
    int x;                  // Posición X en coordenadas de mundo.
    int y;                  // Posición Y en coordenadas de mundo.
    Uint32 createdAt;       // Momento en que se creó el efecto.
    Uint32 durationMs = 500;// Duración total del efecto.
};

// Sistema encargado de:
// - detectar clicks sobre enemigos
// - crear efectos visuales de ataque
// - renderizar dichos efectos
// - más adelante, enviar mensaje real de ataque al servidor
class AttackSystem {
public:
    AttackSystem() = default;

    // Detecta si el click cayó sobre algún enemigo.
    void handleMouseClick(
        int screenX,
        int screenY,
        const SDL_Rect& camera,
        std::map<uint32_t, Entity*>& enemies,
        Queue<std::shared_ptr<const Message>>* sendQueue
    );

    // Borra efectos vencidos.
    void update();

    // Dibuja los efectos activos.
    void render(
        SDL_Renderer* renderer,
        AssetManager& assets,
        const SDL_Rect& camera
    );

private:
    std::vector<AttackEffect> attackEffects;

    // Vida local de enemigos para demo.
    std::unordered_map<uint32_t, int> enemyHealth;

    // Crea el efecto local de ataque sobre el enemigo.
    void createLocalAttackEffect(uint32_t targetId, Entity& target);

    // Aplica daño y devuelve true si el enemigo murió.
    bool applyDamage(uint32_t targetId, int damage);

    // Más adelante acá se reactiva el envío al servidor.
    void sendAttackMessage(
        uint32_t targetId,
        Queue<std::shared_ptr<const Message>>* sendQueue
    );
};

#endif //TALLER_TP_ATTACKSYSTEM_H
