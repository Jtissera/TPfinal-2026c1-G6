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
    const ItemView* equippedWeapon
) {
    (void)equippedWeapon; // ya no se usa para crear efecto local inmediato

    for (const AttackTarget& target : targets) {
        if (target.entity == nullptr) continue;

        SDL_Rect clickableRect{};

        if (target.entity->hasComponent<SpriteComponent>()) {
            const auto& sprite = target.entity->getComponent<SpriteComponent>();
            clickableRect = sprite.getDestRect();
            clickableRect.x -= 10;
            clickableRect.y -= 10;
            clickableRect.w += 20;
            clickableRect.h += 20;
        } else {
            auto& tf = target.entity->getComponent<TransformComponent>();
            clickableRect = SDL_Rect{
                static_cast<int>(tf.position.x - camera.x),
                static_cast<int>(tf.position.y - camera.y + 133),
                96, 96
            };
        }

        const bool clickedTarget =
            screenX >= clickableRect.x &&
            screenX <= clickableRect.x + clickableRect.w &&
            screenY >= clickableRect.y &&
            screenY <= clickableRect.y + clickableRect.h;

        if (!clickedTarget) continue;

        sendAttackMessage(target.id, sendQueue);

        return;
    }
}

void AttackSystem::createAttackEffect(
    uint32_t targetId,
    Entity& target,
    const SDL_Rect& camera,
    AttackEffectType type
) {
    (void) targetId;
    // Para ubicar el efecto usamos el sprite real del target.
    if (!target.hasComponent<SpriteComponent>()) {
        return;
    }

    const auto& sprite = target.getComponent<SpriteComponent>();
    const SDL_Rect& targetRect = sprite.getDestRect();

    // Tamaño base para centrar. La sangre es 32x32 y la magia 64x64,
    // pero usamos 64 como referencia para centrar bien ambos.
    constexpr int effectSize = 64;

    AttackEffect effect{};

    // Convertimos de pantalla a mundo:
    // targetRect está en pantalla, por eso sumamos cámara y restamos offset del mapa.
    effect.x = targetRect.x + targetRect.w / 2 + camera.x - effectSize / 2;
    effect.y = targetRect.y + targetRect.h / 2 + camera.y - 133 - effectSize / 2;

    effect.createdAt = SDL_GetTicks();
    effect.durationMs = 500;
    effect.type = type;

    attackEffects.push_back(effect);
}

void AttackSystem::sendAttackMessage(uint32_t targetId, Queue<std::shared_ptr<const Message>>* sendQueue) {
    if (sendQueue == nullptr) return;
    sendQueue->try_push(std::make_shared<const AttackMessage>(targetId));
    std::cout << "[ATTACK] AttackMessage enviado. targetId=" << targetId << std::endl;
}

void AttackSystem::update() {
    Uint32 now = SDL_GetTicks();
    attackEffects.erase(
        std::remove_if(attackEffects.begin(), attackEffects.end(),
            [now](const AttackEffect& e) { return now - e.createdAt > e.durationMs; }),
        attackEffects.end()
    );
}

void AttackSystem::render(SDL_Renderer* renderer,
                          AssetManager& assets,
                          const SDL_Rect& camera)
{
    Uint32 now = SDL_GetTicks();

    for (auto& ef : attackEffects) {
        Uint32 elapsed = now - ef.createdAt;

        SDL_Texture* texture = nullptr;

        int frameWidth = 0;
        int frameHeight = 0;
        int totalFrames = 0;

        if (ef.type == AttackEffectType::Blood) {
            texture = assets.GetTexture("effect_blood_01");

            frameWidth = 32;
            frameHeight = 32;
            totalFrames = 5;
        } else {
            texture = assets.GetTexture("effect_attack_magic_01");
            frameWidth = 64;
            frameHeight = 64;
            totalFrames = 11;
        }

        if (texture == nullptr) {
            continue;
        }

        int frame = static_cast<int>((elapsed * totalFrames) / ef.durationMs);

        if (frame >= totalFrames) {
            frame = totalFrames - 1;
        }

        if (ef.type == AttackEffectType::Blood) {
            // Sangre: spritesheet horizontal de 5 frames.
            SDL_Rect src{frame * frameWidth,0,frameWidth,frameHeight};

            SDL_Rect dst{
                ef.x - camera.x + 16,       // ajuste para centrar sangre dentro del efectoSize 64
                ef.y - camera.y + 133 + 16,
                32,
                32
            };

            SDL_RenderCopy(renderer, texture, &src, &dst);
        } else {
            // Magia: spritesheet existente de dos filas.
            int srcX = 0;
            int srcY = 0;

            if (frame < 5) {
                srcX = frame * frameWidth;
                srcY = 0;
            } else {
                srcX = (frame - 5) * frameWidth;
                srcY = 64;
            }

            SDL_Rect src{srcX,srcY,frameWidth,frameHeight};

            SDL_Rect dst{
                ef.x - camera.x,
                ef.y - camera.y + 133,
                64,
                64
            };

            SDL_RenderCopy(renderer, texture, &src, &dst);
        }
    }
}

int AttackSystem::getEnemyHealth(uint32_t enemyId) const {
    auto it = enemyHealth.find(enemyId);
    return it == enemyHealth.end() ? 100 : it->second;
}

int AttackSystem::getEnemyMaxHealth(uint32_t enemyId) const {
    auto it = enemyMaxHealth.find(enemyId);
    return it == enemyMaxHealth.end() ? 100 : it->second;
}

bool AttackSystem::isEnemyDead(uint32_t enemyId) const {
    return deadEnemies.find(enemyId) != deadEnemies.end();
}

void AttackSystem::setEnemyHealth(uint32_t enemyId, int hp, int maxHp) {
    if (maxHp < 0) maxHp = 0;
    if (hp < 0) hp = 0;
    if (hp > maxHp) hp = maxHp;

    enemyHealth[enemyId] = hp;
    enemyMaxHealth[enemyId] = maxHp;

    if (hp <= 0) deadEnemies.insert(enemyId);
    else deadEnemies.erase(enemyId);

    std::cout << "[ATTACK SYSTEM] vida NPC actualizada id=" << enemyId
              << " hp=" << hp << "/" << maxHp << std::endl;
}

int AttackSystem::attackRangeForWeapon(const ItemView* weapon) const {
    if (weapon == nullptr) return 35;
    if (weapon->type == ClientItemType::MeleeWeapon) return 55;
    if (weapon->type == ClientItemType::RangedWeapon) return 220;
    if (weapon->type == ClientItemType::MagicWeapon) return 180;
    return 35;
}

bool AttackSystem::isTargetInRange(Entity* attacker, Entity& target, int range) const {
    if (attacker == nullptr) return false;

    auto& attackerTf = attacker->getComponent<TransformComponent>();
    auto& targetTf = target.getComponent<TransformComponent>();

    float dx = (targetTf.position.x + 32.0f) - (attackerTf.position.x + 32.0f);
    float dy = (targetTf.position.y + 64.0f) - (attackerTf.position.y + 64.0f);

    return std::sqrt(dx * dx + dy * dy) <= static_cast<float>(range);
}

bool AttackSystem::shouldCreateVisualEffect(const ItemView* weapon) const {
    if (weapon == nullptr) return false;
    return weapon->type == ClientItemType::MagicWeapon;
}



void AttackSystem::triggerBloodEffect(uint32_t targetId,Entity* targetEntity,const SDL_Rect& camera)
{
    if (targetEntity == nullptr) {
        return;
    }

    createAttackEffect(
        targetId,
        *targetEntity,
        camera,
        AttackEffectType::Blood
    );
}

void AttackSystem::triggerMagicEffect(uint32_t targetId,Entity* targetEntity,const SDL_Rect& camera){
    if (targetEntity == nullptr) {
        return;
    }

    createAttackEffect(
        targetId,
        *targetEntity,
        camera,
        AttackEffectType::Magic
    );
}

void AttackSystem::triggerAttackEffect(uint32_t targetId,Entity* targetEntity,const SDL_Rect& camera,bool isMagicWeapon)
{
    if (targetEntity == nullptr) {
        return;
    }

    if (!isMagicWeapon) {
        return;
    }

    triggerMagicEffect(targetId, targetEntity, camera);
}