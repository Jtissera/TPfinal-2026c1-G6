//
// Created by mauro on 18/5/26.
//

#include "UILabel.h"
#include "../../Game.h"

UILabel::UILabel(int xpos, int ypos, const std::string& text,
                 const std::string& font, SDL_Color colour)
    : labelText(text), labelFont(font), textColour(colour) {
    position.x = xpos;
    position.y = ypos;
    SetLabelText(labelText, labelFont);
}

UILabel::~UILabel() {
    if (labelTexture) {
        SDL_DestroyTexture(labelTexture);
    }
}

void UILabel::SetLabelText(const std::string& text, const std::string& font) {
    if (labelTexture) {
        SDL_DestroyTexture(labelTexture);
        labelTexture = nullptr;
    }
    SDL_Surface* surf = TTF_RenderText_Blended(
        Game::assets->GetFont(font), text.c_str(), textColour);
    if (!surf) return;
    labelTexture = SDL_CreateTextureFromSurface(Game::renderer, surf);
    SDL_FreeSurface(surf);
    SDL_QueryTexture(labelTexture, nullptr, nullptr, &position.w, &position.h);
}

void UILabel::draw() {
    SDL_RenderCopy(Game::renderer, labelTexture, nullptr, &position);
}