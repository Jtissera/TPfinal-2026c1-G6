
#ifndef TALLER_TP_HEALTHBARCOMPONENT_H
#define TALLER_TP_HEALTHBARCOMPONENT_H




#ifndef HEALTH_BAR_COMPONENT_H
#define HEALTH_BAR_COMPONENT_H

#include "ECS.h"
#include "TransformComponent.h"
#include "SpriteComponent.h"

#include <SDL2/SDL.h>
#include <string>

class HealthBarComponent : public Component {
private:
    // Posición lógica de la entidad.
    TransformComponent* transform = nullptr;

    // Sprite visual real. Lo usamos para ubicar la barra sobre el sprite,
    // no sobre el collider lógico.
    SpriteComponent* sprite = nullptr;

    // Vida actual y máxima.
    int hp = 1;
    int hpMax = 1;

    // Textura base de la barra, cargada en AssetManager.
    std::string textureId = "barra_vida";

    // Tamaño visual de la barra sobre entidades.
    int barWidth = 50;
    int barHeight = 10;

    // Offset respecto al sprite.
    int offsetY = -10;

public:
    HealthBarComponent(int hp, int hpMax);

    HealthBarComponent(int hp,
                       int hpMax,
                       int barWidth,
                       int barHeight,
                       int offsetY);

    void init() override;

    void draw(RenderContext& context) override;

    void setHealth(int newHp, int newHpMax);
};

#endif

#endif //TALLER_TP_HEALTHBARCOMPONENT_H
