
#ifndef PRUEBA_SDL_UILABEL_H
#define PRUEBA_SDL_UILABEL_H

#include <string>
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#include "ECS.h"

class UILabel : public Component {
public:
    UILabel(int xpos, int ypos, const std::string& text,
            const std::string& font, SDL_Color colour);
    ~UILabel();

    void SetLabelText(const std::string& text, const std::string& font);
    void draw() override;

private:
    SDL_Rect     position{};
    std::string  labelText;
    std::string  labelFont;
    SDL_Color    textColour{};
    SDL_Texture* labelTexture = nullptr;
};

#endif //PRUEBA_SDL_UILABEL_H
