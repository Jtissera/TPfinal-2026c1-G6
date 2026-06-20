#ifndef NAMEPLATE_COMPONENT_H
#define NAMEPLATE_COMPONENT_H

#include "ECS.h"
#include "TransformComponent.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <cstdint>
#include <string>

// Define qué tipo de entidad está usando el nameplate.
// Esto nos permite cambiar color o formato según sea local, remoto o enemigo.
enum class NameplateType {
    LocalPlayer,
    RemotePlayer,
    Enemy,
    PassiveNpc
};

class NameplateComponent : public Component {
private:
    // Puntero al Transform de la misma entidad.
    // Se usa para saber dónde dibujar el texto.
    TransformComponent* transform = nullptr;

    // Datos visibles.
    std::string name;
    std::string className;
    uint32_t level = 1;
    std::string clan;


    // Tipo de entidad: jugador local, remoto o enemigo.
    NameplateType type = NameplateType::RemotePlayer;
    std::string fontId = "ao_regular";

    struct TextLine {
        SDL_Texture* texture = nullptr;
        int width = 0;
        int height = 0;
    };

    std::vector<TextLine> textLines;

    // Indica si hay que reconstruir la textura.
    bool dirty = true;

    // Arma el texto contiguo que se va a mostrar.
    std::vector<std::string> buildLines() const;

    // Elige color según el tipo de entidad.
    SDL_Color textColor() const;

    // Libera la textura cacheada si existe.
    void destroyTexture();

    // Reconstruye la textura solo cuando cambió el texto.
    void rebuildTexture(RenderContext& context);

public:
    NameplateComponent(std::string name,
                       std::string className,
                       uint32_t level,
                       std::string clan,
                       NameplateType type,
                       std::string fontId);

    ~NameplateComponent() override;

    void init() override;

    void draw(RenderContext& context) override;

    // Métodos para actualizar datos sin recrear el componente.
    void setLevel(uint32_t newLevel);
    void setClan(const std::string& newClan);
    void setName(const std::string& newName);
    void setClassName(const std::string& newClassName);
};

#endif