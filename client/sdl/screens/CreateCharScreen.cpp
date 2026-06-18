#include "CreateCharScreen.h"
#include <stdexcept>


// Datos estaticos
const std::vector<std::string> CreateCharScreen::RAZAS  = {"Human", "Elf", "Dwarf", "Gnome"};
const std::vector<std::string> CreateCharScreen::CLASES = {"Mage", "Paladin", "Cleric", "Warrior"};

// Constructor/Destructor
CreateCharScreen::CreateCharScreen(SDL_Renderer* renderer, int windowW, int windowH,
                                   const std::string& fontPath, Mode mode)
    : renderer(renderer), windowW(windowW), windowH(windowH), mode(mode)
{
    if (TTF_WasInit() == 0 && TTF_Init() == -1)
        throw std::runtime_error(std::string("TTF_Init: ") + TTF_GetError());

    fontLarge  = TTF_OpenFont(fontPath.c_str(), 42);
    fontMedium = TTF_OpenFont(fontPath.c_str(), 24);
    fontSmall  = TTF_OpenFont(fontPath.c_str(), 16);

    if (!fontLarge || !fontMedium || !fontSmall)
        throw std::runtime_error(std::string("TTF_OpenFont: ") + TTF_GetError());

    SDL_StartTextInput();
}

CreateCharScreen::~CreateCharScreen() {
    SDL_StopTextInput();
    if (fontLarge)  { TTF_CloseFont(fontLarge);  fontLarge  = nullptr; }
    if (fontMedium) { TTF_CloseFont(fontMedium); fontMedium = nullptr; }
    if (fontSmall)  { TTF_CloseFont(fontSmall);  fontSmall  = nullptr; }
}

void CreateCharScreen::setError(const std::string& msg) { errorMsg = msg; }

// Loop principal
ScreenResult CreateCharScreen::run() {
    bool dirty = true;
    while (true) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                return ScreenResult::QUIT;

            ScreenResult res;
            if (handleEvent(e, res))
                return res;
            // Si hubo evento, probablemente cambió algo visual.
            dirty = true;
        }
        static Uint32 lastBlinkUpdate = 0;
        Uint32 now = SDL_GetTicks();

        if (now - lastBlinkUpdate >= 500) {
            dirty = true;
            lastBlinkUpdate = now;
        }

        if (dirty) {
            render();
            SDL_RenderPresent(renderer);
            dirty = false;
        }

        SDL_Delay(33);
    }
}

// Input
bool CreateCharScreen::handleEvent(const SDL_Event& e, ScreenResult& out) {
    bool done = false;

    if (e.type == SDL_KEYDOWN) {
        handleKeyDown(e.key.keysym.sym, out, done);
        return done;
    }

    if (e.type == SDL_TEXTINPUT && focus == Focus::NAME) {
        handleTextInput(e.text.text);
        return false;
    }

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x, my = e.button.y;
        const int cx = windowW / 2;
        const int baseY = (mode == Mode::CREATE) ? 140 : 160;

        // Campo nombre
        SDL_Rect nameBox = {cx - 180, baseY + 36, 360, 38};
        if (isClickOn(nameBox, mx, my)) { focus = Focus::NAME; return false; }

        if (mode == Mode::CREATE) {
            // Raza — flecha izq/der
            SDL_Rect razaLeft  = {cx - 160, baseY + 130, 30, 32};
            SDL_Rect razaRight = {cx + 130, baseY + 130, 30, 32};
            if (isClickOn(razaLeft,  mx, my)) { focus = Focus::RAZA; cycleOption(-1); return false; }
            if (isClickOn(razaRight, mx, my)) { focus = Focus::RAZA; cycleOption(+1); return false; }

            // Clase — flecha izq/der
            SDL_Rect claseLeft  = {cx - 160, baseY + 210, 30, 32};
            SDL_Rect claseRight = {cx + 130, baseY + 210, 30, 32};
            if (isClickOn(claseLeft,  mx, my)) { focus = Focus::CLASE; cycleOption(-1); return false; }
            if (isClickOn(claseRight, mx, my)) { focus = Focus::CLASE; cycleOption(+1); return false; }
        }

        // Botones
        int btnY = (mode == Mode::CREATE) ? baseY + 290 : baseY + 150;
        SDL_Rect btnOk   = {cx - 140, btnY, 120, 42};
        SDL_Rect btnBack = {cx + 20,  btnY, 120, 42};

        if (isClickOn(btnOk, mx, my)) {
            focus = Focus::CONFIRM;
            out = confirm();
            return (out != ScreenResult::STAY);
        }
        if (isClickOn(btnBack, mx, my)) {
            out = ScreenResult::GO_MAIN_MENU;
            return true;
        }
    }

    return false;
}

void CreateCharScreen::handleKeyDown(SDL_Keycode key, ScreenResult& out, bool& done) {
    switch (key) {
        case SDLK_TAB:
            cycleFocus(1);
            break;
        case SDLK_UP:   case SDLK_LEFT:
            if (focus == Focus::RAZA || focus == Focus::CLASE) cycleOption(-1);
            else cycleFocus(-1);
            break;
        case SDLK_DOWN: case SDLK_RIGHT:
            if (focus == Focus::RAZA || focus == Focus::CLASE) cycleOption(+1);
            else cycleFocus(+1);
            break;
        case SDLK_BACKSPACE:
            if (focus == Focus::NAME) handleBackspace();
            break;
        case SDLK_RETURN: case SDLK_KP_ENTER:
            if (focus == Focus::BACK) {
                out = ScreenResult::GO_MAIN_MENU;
                done = true;
            } else {
                out = confirm();
                done = (out != ScreenResult::STAY);
            }
            break;
        case SDLK_ESCAPE:
            out = ScreenResult::GO_MAIN_MENU;
            done = true;
            break;
        default: break;
    }
}

void CreateCharScreen::handleTextInput(const char* text) {
    if (username.size() < 20)
        username += text;
    errorMsg.clear();
}

void CreateCharScreen::handleBackspace() {
    if (!username.empty()) username.pop_back();
    errorMsg.clear();
}

void CreateCharScreen::cycleFocus(int delta) {
    const int maxFocus = (mode == Mode::CREATE) ? 4 : 2;  // NAME,RAZA,CLASE,CONFIRM,BACK vs NAME,CONFIRM,BACK
    int f = static_cast<int>(focus);

    if (mode == Mode::LOGIN) {
        // Solo NAME(0), CONFIRM(3→1), BACK(4→2)
        static const Focus loginOrder[] = {Focus::NAME, Focus::CONFIRM, Focus::BACK};
        int idx = 0;
        for (int i = 0; i < 3; ++i) if (loginOrder[i] == focus) { idx = i; break; }
        idx = (idx + delta + 3) % 3;
        focus = loginOrder[idx];
    } else {
        static const Focus createOrder[] = {Focus::NAME, Focus::RAZA, Focus::CLASE, Focus::CONFIRM, Focus::BACK};
        int idx = 0;
        for (int i = 0; i < 5; ++i) if (createOrder[i] == focus) { idx = i; break; }
        idx = (idx + delta + 5) % 5;
        focus = createOrder[idx];
    }
    (void)f; (void)maxFocus; (void)delta;
}

void CreateCharScreen::cycleOption(int delta) {
    if (focus == Focus::RAZA) {
        razaIdx = (razaIdx + delta + static_cast<int>(RAZAS.size())) % static_cast<int>(RAZAS.size());
    } else if (focus == Focus::CLASE) {
        claseIdx = (claseIdx + delta + static_cast<int>(CLASES.size())) % static_cast<int>(CLASES.size());
    }
}

ScreenResult CreateCharScreen::confirm() {
    if (username.empty()) {
        errorMsg = "El nombre no puede estar vacio.";
        focus = Focus::NAME;
        return ScreenResult::STAY;
    }

    return ScreenResult::GO_LOBBY;
}

bool CreateCharScreen::isClickOn(const SDL_Rect& r, int mx, int my) const {
    return mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h;
}

// Renderizado
void CreateCharScreen::render() {
    SDL_SetRenderDrawColor(renderer, C_BG.r, C_BG.g, C_BG.b, 255);
    SDL_RenderClear(renderer);
    renderTitle();
    renderNameField();
    if (mode == Mode::CREATE) {
        const int baseY = 140;
        renderSelector("Raza",  RAZAS,  razaIdx,  windowW/2 - 160, baseY + 110, 320, focus == Focus::RAZA);
        renderSelector("Clase", CLASES, claseIdx, windowW/2 - 160, baseY + 190, 320, focus == Focus::CLASE);
    }
    renderButtons();
    renderError();
}

void CreateCharScreen::renderTitle() {
    const char* title = (mode == Mode::CREATE) ? "Crear Personaje" : "Iniciar Sesion";
    SDL_Rect dst{};
    SDL_Texture* tex = makeText(title, fontLarge, C_TITLE, dst);
    if (!tex) return;
    dst.x = windowW / 2 - dst.w / 2;
    dst.y = 40;
    drawTex(tex, dst);
    SDL_DestroyTexture(tex);
}

void CreateCharScreen::renderNameField() {
    const int baseY = (mode == Mode::CREATE) ? 140 : 160;
    const int cx = windowW / 2;

    SDL_Rect lblR{};
    SDL_Texture* lbl = makeText("Nombre:", fontSmall, C_LABEL, lblR);
    if (lbl) {
        lblR.x = cx - 180; lblR.y = baseY + 8;
        drawTex(lbl, lblR); SDL_DestroyTexture(lbl);
    }

    SDL_Rect box = {cx - 180, baseY + 36, 360, 38};
    SDL_Color boxColor = (focus == Focus::NAME) ? C_ACTIVE_BG : SDL_Color{30, 28, 50, 200};
    drawRect(box, boxColor, true);
    SDL_Color borderColor = (focus == Focus::NAME) ? C_ACTIVE : C_INACTIVE;
    SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, 255);
    SDL_RenderDrawRect(renderer, &box);

    std::string display = username;
    if (focus == Focus::NAME && (SDL_GetTicks() / 500) % 2 == 0) display += "|";

    if (!display.empty()) {
        SDL_Rect txtR{};
        SDL_Texture* txt = makeText(display, fontMedium, C_TEXT, txtR);
        if (txt) {
            txtR.x = box.x + 8;
            txtR.y = box.y + (box.h - txtR.h) / 2;
            drawTex(txt, txtR); SDL_DestroyTexture(txt);
        }
    }
}

void CreateCharScreen::renderSelector(const char* label, const std::vector<std::string>& opts,
                                       int selected, int x, int y, int w, bool active) {
    SDL_Rect lblR{};
    SDL_Texture* lbl = makeText(label, fontSmall, C_LABEL, lblR);
    if (lbl) { lblR.x = x; lblR.y = y; drawTex(lbl, lblR); SDL_DestroyTexture(lbl); }

    // Fila de seleccion: < opción >
    SDL_Rect row = {x, y + 24, w, 34};
    SDL_Color bg = active ? C_ACTIVE_BG : SDL_Color{30, 28, 50, 180};
    drawRect(row, bg, true);
    SDL_Color border = active ? C_ACTIVE : C_INACTIVE;
    SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, 255);
    SDL_RenderDrawRect(renderer, &row);

    // Flecha izq
    SDL_Rect arrowL{};
    SDL_Texture* aL = makeText("<", fontMedium, active ? C_ACTIVE : C_INACTIVE, arrowL);
    if (aL) { arrowL.x = row.x + 10; arrowL.y = row.y + (row.h - arrowL.h)/2; drawTex(aL, arrowL); SDL_DestroyTexture(aL); }

    // Opcion seleccionada
    SDL_Rect optR{};
    SDL_Texture* optTex = makeText(opts[selected], fontMedium, active ? C_ACTIVE : C_TEXT, optR);
    if (optTex) { optR.x = row.x + (row.w - optR.w)/2; optR.y = row.y + (row.h - optR.h)/2; drawTex(optTex, optR); SDL_DestroyTexture(optTex); }

    // Flecha der
    SDL_Rect arrowR{};
    SDL_Texture* aR = makeText(">", fontMedium, active ? C_ACTIVE : C_INACTIVE, arrowR);
    if (aR) { arrowR.x = row.x + row.w - arrowR.w - 10; arrowR.y = row.y + (row.h - arrowR.h)/2; drawTex(aR, arrowR); SDL_DestroyTexture(aR); }
}

void CreateCharScreen::renderButtons() {
    const int cx = windowW / 2;
    const int baseY = (mode == Mode::CREATE) ? 140 : 160;
    const int btnY  = (mode == Mode::CREATE) ? baseY + 290 : baseY + 150;

    const char* okLabel = (mode == Mode::CREATE) ? "Crear" : "Ingresar";

    SDL_Rect btnOk = {cx - 140, btnY, 120, 42};
    SDL_Color okBg = (focus == Focus::CONFIRM) ? SDL_Color{60, 180, 90, 220} : C_BTN_OK;
    drawRect(btnOk, okBg, true);
    SDL_SetRenderDrawColor(renderer, 80, 200, 100, 255);
    SDL_RenderDrawRect(renderer, &btnOk);
    SDL_Rect okTxtR{};
    SDL_Texture* okTex = makeText(okLabel, fontMedium, C_TEXT, okTxtR);
    if (okTex) { okTxtR.x = btnOk.x + (btnOk.w - okTxtR.w)/2; okTxtR.y = btnOk.y + (btnOk.h - okTxtR.h)/2; drawTex(okTex, okTxtR); SDL_DestroyTexture(okTex); }

    SDL_Rect btnBack = {cx + 20, btnY, 120, 42};
    SDL_Color backBg = (focus == Focus::BACK) ? SDL_Color{150, 60, 60, 220} : C_BTN_BACK;
    drawRect(btnBack, backBg, true);
    SDL_SetRenderDrawColor(renderer, 180, 80, 80, 255);
    SDL_RenderDrawRect(renderer, &btnBack);
    SDL_Rect backTxtR{};
    SDL_Texture* backTex = makeText("Volver", fontMedium, C_TEXT, backTxtR);
    if (backTex) { backTxtR.x = btnBack.x + (btnBack.w - backTxtR.w)/2; backTxtR.y = btnBack.y + (btnBack.h - backTxtR.h)/2; drawTex(backTex, backTxtR); SDL_DestroyTexture(backTex); }

    SDL_Rect footer{};
    SDL_Texture* ft = makeText("Tab para navegar  |  Enter para confirmar  |  Esc para volver", fontSmall, {80,80,80,255}, footer);
    if (ft) { footer.x = cx - footer.w/2; footer.y = windowH - 32; drawTex(ft, footer); SDL_DestroyTexture(ft); }
}

void CreateCharScreen::renderError() {
    if (errorMsg.empty()) return;
    SDL_Rect dst{};
    SDL_Texture* tex = makeText(errorMsg, fontSmall, C_ERROR, dst);
    if (!tex) return;
    dst.x = windowW / 2 - dst.w / 2;
    dst.y = windowH - 60;
    drawTex(tex, dst);
    SDL_DestroyTexture(tex);
}

// Helpers
SDL_Texture* CreateCharScreen::makeText(const std::string& text, TTF_Font* font,
                                         SDL_Color color, SDL_Rect& out) {
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surf) return nullptr;
    out.w = surf->w; out.h = surf->h;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
    return tex;
}

void CreateCharScreen::drawTex(SDL_Texture* tex, const SDL_Rect& dst) {
    SDL_RenderCopy(renderer, tex, nullptr, &dst);
}

void CreateCharScreen::drawRect(const SDL_Rect& r, SDL_Color color, bool fill) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    if (fill) SDL_RenderFillRect(renderer, &r);
    else       SDL_RenderDrawRect(renderer, &r);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}
