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
#include "world/RemotePlayer.h"

enum class AttackEffectType
{
    Blood,
    Magic
};

// Representa un efecto visual de ataque activo.
// Por ahora solo guarda posición, tiempo de creación y duración.
struct AttackEffect
{
    int x;                   // Posición X en coordenadas de mundo.
    int y;                   // Posición Y en coordenadas de mundo.
    Uint32 createdAt;        // Momento en que se creó el efecto.
    Uint32 durationMs = 500; // Duración total del efecto.
    AttackEffectType type = AttackEffectType::Blood;
};

struct AttackTarget
{
    uint32_t id;
    Entity *entity;
};

// Resultado de la actualización de persecución de enemigos.
// Sirve para que Game sepa si el jugador murió durante el ataque enemigo.
enum class EnemyChaseResult
{
    PlayerStillAlive, // El enemigo actualizó persecución/ataque y el jugador sigue vivo.
    PlayerDied        // Algún enemigo atacó y la vida del jugador llegó a 0.
};

// Sistema encargado de:
// - detectar clicks sobre enemigos
// - crear efectos visuales de ataque
// - renderizar dichos efectos
// - más adelante, enviar mensaje real de ataque al servidor
class AttackSystem
{
public:
    AttackSystem() = default;

    // Detecta click sobre enemigo, manda mensaje al servidor
    // y crea efecto visual si corresponde.
    bool handleMouseClick(
        int screenX,
        int screenY,
        const SDL_Rect &camera,
        const std::vector<AttackTarget> &targets,
        Queue<std::shared_ptr<const Message>> *sendQueue,
        const ItemView *equippedWeapon);

    // Crea efecto visual de ataque sobre un enemigo.

    // Elimina efectos vencidos.
    void update();

    // Dibuja los efectos activos.
    void render(SDL_Renderer *renderer, AssetManager &assets, const SDL_Rect &camera);

    // Indica si el enemigo está muerto temporalmente.
    bool isEnemyDead(uint32_t enemyId) const;

    // Vida actual del enemigo.
    int getEnemyHealth(uint32_t enemyId) const;

    // Vida máxima del enemigo.
    int getEnemyMaxHealth(uint32_t enemyId) const;

    // Actualiza la vida del enemigo desde el servidor.
    void setEnemyHealth(uint32_t enemyId, int hp, int maxHp);

    void triggerAttackEffect(uint32_t targetId, Entity *targetEntity, const SDL_Rect &camera, bool isMagicWeapon);

    void triggerBloodEffect(uint32_t targetId, Entity *targetEntity, const SDL_Rect &camera);

    void triggerMagicEffect(uint32_t targetId, Entity *targetEntity, const SDL_Rect &camera);

private:
    std::vector<AttackEffect> attackEffects;

    std::unordered_map<uint32_t, int> enemyHealth;
    std::unordered_map<uint32_t, int> enemyMaxHealth;
    std::unordered_set<uint32_t> deadEnemies;

    void sendAttackMessage(uint32_t targetId, Queue<std::shared_ptr<const Message>> *sendQueue);
    int attackRangeForWeapon(const ItemView *weapon) const;
    bool isTargetInRange(Entity *attacker, Entity &target, int range) const;
    bool shouldCreateVisualEffect(const ItemView *weapon) const;
    void createAttackEffect(uint32_t targetId, Entity &target, const SDL_Rect &camera, AttackEffectType type);
};

#endif
