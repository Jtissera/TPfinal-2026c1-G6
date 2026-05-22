#include "MainMenuScreen.h"

#include <stdexcept>
#include <iostream>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────

MainMenuScreen::MainMenuScreen(SDL_Renderer* renderer, int windowW, int windowH,
                               const std::string& fontPath)
    : renderer(renderer), windowW(windowW), windowH(windowH)
{
    if (TTF_WasInit() == 0) {
        if (TTF_Init() == -1)
            throw std::runtime_error(std::string("TTF_Init: ") + TTF_GetError());
    }

    fontLarge  = TTF_OpenFont(fontPath.c_str(), 52);
    fontMedium = TTF_OpenFont(fontPath.c_str(), 28);
    fontSmall  = TTF_OpenFont(fontPath.c_str(), 18);

    if (!fontLarge || !fontMedium || !fontSmall)
        throw std::runtime_error(std::string("TTF_OpenFont: ") + TTF_GetError());

    items = {
        {"Crear personaje", ScreenResult::GO_CREATE_CHAR},
        {"Iniciar sesion",  ScreenResult::GO_LOGIN},
        {"Configuracion",   ScreenResult::GO_CONFIG},
        {"Salir",           ScreenResult::QUIT},
    };
}

MainMenuScreen::~MainMenuScreen() {
    if (fontLarge)  { TTF_CloseFont(fontLarge);  fontLarge  = nullptr; }
    if (fontMedium) { TTF_CloseFont(fontMedium); fontMedium = nullptr; }
    if (fontSmall)  { TTF_CloseFont(fontSmall);  fontSmall  = nullptr; }
}

void MainMenuScreen::setError(const std::string& msg) {
    errorMsg = msg;
}

// ─────────────────────────────────────────────────────────────────────────────
// Loop principal
// ─────────────────────────────────────────────────────────────────────────────

ScreenResult MainMenuScreen::run() {
    while (true) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                return ScreenResult::QUIT;

            ScreenResult res;
            if (handleEvent(e, res))
                return res;
        }

        render();
        SDL_RenderPresent(renderer);
        SDL_Delay(16);  // ~60 fps
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Input
// ─────────────────────────────────────────────────────────────────────────────

bool MainMenuScreen::handleEvent(const SDL_Event& e, ScreenResult& out) {
    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
            case SDLK_UP:
            case SDLK_w:
                moveSelection(-1);
                break;
            case SDLK_DOWN:
            case SDLK_s:
                moveSelection(+1);
                break;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
            case SDLK_SPACE:
                out = items[selectedIndex].result;
                return true;
            case SDLK_ESCAPE:
                out = ScreenResult::QUIT;
                return true;
            default:
                break;
        }
    }

    // Click izquierdo
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        const int itemH   = 52;
        const int spacing = 18;
        const int totalH  = static_cast<int>(items.size()) * (itemH + spacing) - spacing;
        const int startY  = windowH / 2 - totalH / 2 + 40;
        const int mx = e.button.x;
        const int my = e.button.y;

        for (int i = 0; i < static_cast<int>(items.size()); ++i) {
            SDL_Rect row = {windowW / 2 - 180, startY + i * (itemH + spacing), 360, itemH};
            if (mx >= row.x && mx <= row.x + row.w &&
                my >= row.y && my <= row.y + row.h) {
                selectedIndex = i;
                out = items[i].result;
                return true;
            }
        }
    }

    // Hover: actualiza selección visualmente
    if (e.type == SDL_MOUSEMOTION) {
        const int itemH   = 52;
        const int spacing = 18;
        const int totalH  = static_cast<int>(items.size()) * (itemH + spacing) - spacing;
        const int startY  = windowH / 2 - totalH / 2 + 40;
        const int mx = e.motion.x;
        const int my = e.motion.y;

        for (int i = 0; i < static_cast<int>(items.size()); ++i) {
            SDL_Rect row = {windowW / 2 - 180, startY + i * (itemH + spacing), 360, itemH};
            if (mx >= row.x && mx <= row.x + row.w &&
                my >= row.y && my <= row.y + row.h) {
                selectedIndex = i;
                break;
            }
        }
    }

    return false;
}

void MainMenuScreen::moveSelection(int delta) {
    selectedIndex = (selectedIndex + delta + static_cast<int>(items.size()))
                    % static_cast<int>(items.size());
}

// ─────────────────────────────────────────────────────────────────────────────
// Renderizado
// ─────────────────────────────────────────────────────────────────────────────

void MainMenuScreen::render() {
    SDL_SetRenderDrawColor(renderer,
        COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, COLOR_BG.a);
    SDL_RenderClear(renderer);

    renderTitle();
    renderMenuItems();
    renderError();
}

void MainMenuScreen::renderTitle() {
    SDL_Rect dst{};
    SDL_Texture* tex = makeTextTexture("Argentum Online", fontLarge, COLOR_TITLE, dst);
    if (!tex) return;
    dst.x = windowW / 2 - dst.w / 2;
    dst.y = windowH / 4 - dst.h / 2;
    drawTexture(tex, dst);
    SDL_DestroyTexture(tex);

    // Subtítulo decorativo
    SDL_Color dim = {160, 140, 60, 255};
    SDL_Rect sub{};
    SDL_Texture* subTex = makeTextTexture("Un mundo de magia y aventura", fontSmall, dim, sub);
    if (subTex) {
        sub.x = windowW / 2 - sub.w / 2;
        sub.y = dst.y + dst.h + 10;
        drawTexture(subTex, sub);
        SDL_DestroyTexture(subTex);
    }
}

void MainMenuScreen::renderMenuItems() {
    const int itemH   = 52;
    const int spacing = 18;
    const int totalH  = static_cast<int>(items.size()) * (itemH + spacing) - spacing;
    const int startY  = windowH / 2 - totalH / 2 + 40;

    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        SDL_Rect row = {windowW / 2 - 180, startY + i * (itemH + spacing), 360, itemH};
        bool selected = (i == selectedIndex);

        if (selected) {
            drawRect(row, COLOR_SELECTED_BG, true);
            SDL_SetRenderDrawColor(renderer,
                COLOR_SELECTED.r, COLOR_SELECTED.g, COLOR_SELECTED.b, 180);
            SDL_RenderDrawRect(renderer, &row);

            // Flecha
            SDL_Rect arrow{};
            SDL_Texture* arrTex = makeTextTexture(">", fontMedium, COLOR_SELECTED, arrow);
            if (arrTex) {
                arrow.x = row.x + 14;
                arrow.y = row.y + (row.h - arrow.h) / 2;
                drawTexture(arrTex, arrow);
                SDL_DestroyTexture(arrTex);
            }
        }

        SDL_Color color = selected ? COLOR_SELECTED : COLOR_ITEM;
        SDL_Rect txt{};
        SDL_Texture* tex = makeTextTexture(items[i].label, fontMedium, color, txt);
        if (!tex) continue;
        txt.x = row.x + (row.w - txt.w) / 2;
        txt.y = row.y + (row.h - txt.h) / 2;
        drawTexture(tex, txt);
        SDL_DestroyTexture(tex);
    }

    // Footer de ayuda
    SDL_Rect footer{};
    SDL_Texture* ftex = makeTextTexture(
        "Flechas / WASD para navegar  |  Enter para confirmar  |  Click para seleccionar",
        fontSmall, COLOR_FOOTER, footer);
    if (ftex) {
        footer.x = windowW / 2 - footer.w / 2;
        footer.y = windowH - 36;
        drawTexture(ftex, footer);
        SDL_DestroyTexture(ftex);
    }
}

void MainMenuScreen::renderError() {
    if (errorMsg.empty()) return;

    SDL_Rect dst{};
    SDL_Texture* tex = makeTextTexture(errorMsg, fontSmall, COLOR_ERROR, dst);
    if (!tex) return;
    dst.x = windowW / 2 - dst.w / 2;
    dst.y = windowH - 70;
    drawTexture(tex, dst);
    SDL_DestroyTexture(tex);
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

SDL_Texture* MainMenuScreen::makeTextTexture(const std::string& text, TTF_Font* font,
                                              SDL_Color color, SDL_Rect& outRect) {
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surf) return nullptr;
    outRect.w = surf->w;
    outRect.h = surf->h;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
    return tex;
}

void MainMenuScreen::drawTexture(SDL_Texture* tex, const SDL_Rect& dst) {
    SDL_RenderCopy(renderer, tex, nullptr, &dst);
}

void MainMenuScreen::drawRect(const SDL_Rect& r, SDL_Color color, bool fill) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    if (fill) SDL_RenderFillRect(renderer, &r);
    else       SDL_RenderDrawRect(renderer, &r);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}
