#ifndef TALLER_TP_ATTACKSYSTEM_H
#define TALLER_TP_ATTACKSYSTEM_H

#include <SDL_render.h>
#include "ECS/Components.h"
#include "AssetManager.h"
#include <SDL2/SDL_rect.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cmath>
#include "state/ItemView.h"

struct AttackEffect {
    int x;
    int y;
    Uint32 createdAt;
    Uint32 durationMs = 500;
};

struct AttackTarget {
    uint32_t id;
    Entity* entity;
};

class AttackSystem {
public:
    AttackSystem() = default;

    // Detecta click sobre enemigo, manda mensaje al servidor
    // y crea efecto visual si corresponde.
    void handleMouseClick(
        int screenX,
        int screenY,
        const SDL_Rect& camera,
        const std::vector<AttackTarget>& targets,
        Queue<std::shared_ptr<const Message>>* sendQueue,
        Entity* player,
        const ItemView* equippedWeapon
    );

    // Crea efecto visual de ataque sobre un enemigo.
    void createLocalAttackEffect(uint32_t targetId, Entity& target, const SDL_Rect& camera);

    // Elimina efectos vencidos.
    void update();

    // Dibuja efectos activos.
    void render(SDL_Renderer* renderer, AssetManager& assets, const SDL_Rect& camera);

    // Vida actual del enemigo.
    int getEnemyHealth(uint32_t enemyId) const;

    // Vida máxima del enemigo.
    int getEnemyMaxHealth(uint32_t enemyId) const;

    // Indica si el enemigo está muerto.
    bool isEnemyDead(uint32_t enemyId) const;

    // Actualiza la vida del enemigo desde el servidor.
    void setEnemyHealth(uint32_t enemyId, int hp, int maxHp);

private:
    std::vector<AttackEffect> attackEffects;

    std::unordered_map<uint32_t, int> enemyHealth;
    std::unordered_map<uint32_t, int> enemyMaxHealth;
    std::unordered_set<uint32_t> deadEnemies;

    void sendAttackMessage(uint32_t targetId, Queue<std::shared_ptr<const Message>>* sendQueue);
    int attackRangeForWeapon(const ItemView* weapon) const;
    bool isTargetInRange(Entity* attacker, Entity& target, int range) const;
    bool shouldCreateVisualEffect(const ItemView* weapon) const;
};

#endif