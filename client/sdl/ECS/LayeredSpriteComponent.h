//
// Created by mauro on 24/5/26.
//

#ifndef TALLER_TP_LAYEREDSPRITECOMPONENT_H
#define TALLER_TP_LAYEREDSPRITECOMPONENT_H
#include <SDL_render.h>
#include <string>

#include "TransformComponent.h"
struct SpriteLayer {
    SDL_Texture* texture;      // Textura de esta capa.
    std::string textureId;     // Id lógico: "body_human", "head_elf_01", etc.
    bool visible = true;       // Permite ocultar capa si no hay casco/escudo.
};

// class LayeredSpriteComponent : public Component {
// private:
//     TransformComponent* transform;
//
//     std::vector<SpriteLayer> layers;
//
//     int frameWidth;
//     int frameHeight;
//     int scale;
//
//     int currentFrame = 0;
//     int animationIndex = 0;
//     int frames = 1;
//     int speed = 100;
//
// public:
//     void addLayer(const std::string& textureId);
//     void changeLayer(int layerIndex, const std::string& newTextureId);
//     void Play(const std::string& animationName);
//     void update() override;
//     void draw() override;
// };
#endif //TALLER_TP_LAYEREDSPRITECOMPONENT_H
