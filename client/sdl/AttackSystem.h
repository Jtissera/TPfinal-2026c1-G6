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
#include <unordered_set>
#include <vector>
#include <cmath>

#include "state/ItemView.h"

// Representa un efecto visual de ataque activo.
// Por ahora solo guarda posición, tiempo de creación y duración.
struct AttackEffect {
    int x;                  // Posición X en coordenadas de mundo.
    int y;                  // Posición Y en coordenadas de mundo.
    Uint32 createdAt;       // Momento en que se creó el efecto.
    Uint32 durationMs = 500;// Duración total del efecto.
};

// Resultado de la actualización de persecución de enemigos.
// Sirve para que Game sepa si el jugador murió durante el ataque enemigo.
enum class EnemyChaseResult {
    PlayerStillAlive, // El enemigo actualizó persecución/ataque y el jugador sigue vivo.
    PlayerDied        // Algún enemigo atacó y la vida del jugador llegó a 0.
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
        Queue<std::shared_ptr<const Message>>* sendQueue,
        Entity* player,
        const ItemView* equippedWeapon
    );

    // Borra efectos vencidos.
    void update();

    // Dibuja los efectos activos.
    void render(SDL_Renderer* renderer,AssetManager& assets,const SDL_Rect& camera);

    // Indica si el enemigo está muerto temporalmente.
    bool isEnemyDead(uint32_t enemyId) const;

    // Actualiza respawns de enemigos muertos.
    // Recibe enemies para poder restaurar la posición original del enemigo.
    void updateRespawns(std::map<uint32_t, Entity*>& enemies);

    // Vida actual del enemigo.
    int getEnemyHealth(uint32_t enemyId) const;

    // Vida máxima del enemigo.
    int getEnemyMaxHealth(uint32_t enemyId) const;

    // Actualiza persecución y ataque del enemigo al jugador.
    EnemyChaseResult updateEnemyChase(std::map<uint32_t, Entity*>& enemies,Entity* player,int& playerHp,Queue<std::shared_ptr<const Message>>* sendQueue);

    // Limpia toda persecución/aggro de enemigos.
    // Se usa cuando el jugador muere o pasa a estado fantasma.
    void clearEnemyAggro();

private:
    std::vector<AttackEffect> attackEffects;

    // Vida local de enemigos para demo.
    std::unordered_map<uint32_t, int> enemyHealth;

    // Vida máxima de enemigos.
    std::unordered_map<uint32_t, int> enemyMaxHealth;

    // Enemigos muertos esperando respawn.
    std::unordered_set<uint32_t> deadEnemies;

    // Momento en que murió cada enemigo.
    std::unordered_map<uint32_t, Uint32> enemyDeadAt;

    // Tiempo de respawn.
    Uint32 enemyRespawnMs = 5000;

    std::unordered_set<uint32_t> chasingEnemies;

    // Posición original del enemigo para respawn visual.
    std::unordered_map<uint32_t, Vector2D> enemySpawnPositions;

    // Velocidad local visual de persecución.
    // Después esto debe venir del TOML/config.
    float enemyChaseSpeed = 2.0f;

    // Distancia mínima para que no se meta encima del jugador.
    float enemyStopDistance = 35.0f;

    Uint32 enemyAttackCooldownMs = 1000;

    int enemyAttackDamage = 5;

    // Último momento en que atacó cada enemigo.
    std::unordered_map<uint32_t, Uint32> enemyLastAttackAt;

    void markEnemyAsDead(uint32_t enemyId);

    // Crea el efecto local de ataque sobre el enemigo.
    void createLocalAttackEffect(uint32_t targetId, Entity& target);

    // Aplica daño y devuelve true si el enemigo murió.
    bool applyDamage(uint32_t targetId, int damage);

    // Más adelante acá se reactiva el envío al servidor.
    void sendAttackMessage(uint32_t targetId,Queue<std::shared_ptr<const Message>>* sendQueu);

    int attackRangeForWeapon(const ItemView* weapon) const;

    int damageForWeapon(const ItemView* weapon) const;

    bool isTargetInRange(Entity* attacker,Entity& target,int range) const;

    bool shouldCreateVisualEffect(const ItemView* weapon) const;
};

#endif //TALLER_TP_ATTACKSYSTEM_H
