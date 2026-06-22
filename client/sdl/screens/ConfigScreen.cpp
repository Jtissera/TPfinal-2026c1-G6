#include "ConfigScreen.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>

static std::string configPath()
{
    const char *home = std::getenv("HOME");
    if (!home)
        home = ".";
    return std::string(home) + "/.config/argentum/client.toml";
}

ClientConfig ClientConfig::load()
{
    ClientConfig cfg;
    std::ifstream f(configPath());
    if (!f.is_open())
        return cfg;

    std::string line;
    while (std::getline(f, line))
    {
        if (line.empty() || line[0] == '#' || line[0] == '[')
            continue;
        auto eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        auto trim = [](std::string &s)
        {
            while (!s.empty() && (s.front() == ' ' || s.front() == '\t'))
                s.erase(s.begin());
            while (!s.empty() && (s.back() == ' ' || s.back() == '\t' ||
                                  s.back() == '\r' || s.back() == '\n'))
                s.pop_back();
        };
        trim(key);
        trim(val);

        try
        {
            if (key == "music_volume")
                cfg.musicVolume = std::stoi(val);
            if (key == "sfx_volume")
                cfg.sfxVolume = std::stoi(val);
            if (key == "fullscreen")
                cfg.fullscreen = (val == "true");
        }
        catch (...)
        {
        }
    }
    return cfg;
}

void ClientConfig::save() const
{
    const char *home = std::getenv("HOME");
    if (!home)
        return;
    std::string dir = std::string(home) + "/.config/argentum";
    mkdir(dir.c_str(), 0755);

    std::ofstream f(configPath());
    if (!f.is_open())
        return;

    f << "# Argentum Online — configuracion del cliente\n";
    f << "# Generado automaticamente. Editar con cuidado.\n\n";
    f << "[client]\n";
    f << "music_volume = " << musicVolume << "\n";
    f << "sfx_volume   = " << sfxVolume << "\n";
    f << "fullscreen   = " << (fullscreen ? "true" : "false") << "\n";
}

// Constructor / Destructor

ConfigScreen::ConfigScreen(SDL_Renderer *renderer, SDL_Window *window,
                           int windowW, int windowH,
                           const std::string &fontPath,
                           ClientConfig &config)
    : renderer(renderer), window(window),
      windowW(windowW), windowH(windowH),
      config(config), original(config)
{
    if (TTF_WasInit() == 0 && TTF_Init() == -1)
        throw std::runtime_error(std::string("TTF_Init: ") + TTF_GetError());

    fontTitle = TTF_OpenFont(fontPath.c_str(), 32);
    fontMedium = TTF_OpenFont(fontPath.c_str(), 20);
    fontSmall = TTF_OpenFont(fontPath.c_str(), 15);

    if (!fontTitle || !fontMedium || !fontSmall)
        throw std::runtime_error(std::string("TTF_OpenFont: ") + TTF_GetError());

    computeLayout();
}

ConfigScreen::~ConfigScreen()
{
    if (fontTitle)
    {
        TTF_CloseFont(fontTitle);
        fontTitle = nullptr;
    }
    if (fontMedium)
    {
        TTF_CloseFont(fontMedium);
        fontMedium = nullptr;
    }
    if (fontSmall)
    {
        TTF_CloseFont(fontSmall);
        fontSmall = nullptr;
    }
}

void ConfigScreen::computeLayout()
{
    const int panelW = 560;
    const int panelH = windowH - 100;
    panel = {(windowW - panelW) / 2, 50, panelW, panelH};
    optStartY = panel.y + 90;
}

// Loop principal

ScreenResult ConfigScreen::run()
{
    while (true)
    {
        // Hover de botones
        int mx, my;
        SDL_GetMouseState(&mx, &my);

        const int btnW = (panel.w - 30) / 2;
        const int btnY = panel.y + panel.h - 50;
        SDL_Rect bSave = {panel.x + 10, btnY, btnW, 38};
        SDL_Rect bCancel = {panel.x + 20 + btnW, btnY, btnW, 38};

        hoveredSave = (mx >= bSave.x && mx <= bSave.x + bSave.w && my >= bSave.y && my <= bSave.y + bSave.h);
        hoveredCancel = (mx >= bCancel.x && mx <= bCancel.x + bCancel.w && my >= bCancel.y && my <= bCancel.y + bCancel.h);

        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                return ScreenResult::QUIT;
            ScreenResult res;
            if (handleEvent(e, res))
                return res;
        }

        render();
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
}

// Input

bool ConfigScreen::handleEvent(const SDL_Event &e, ScreenResult &out)
{
    if (e.type == SDL_KEYDOWN)
    {
        switch (e.key.keysym.sym)
        {
        case SDLK_ESCAPE:
            config = original;
            applyConfig();
            out = ScreenResult::GO_MAIN_MENU;
            return true;

        case SDLK_UP:
        case SDLK_w:
            selectedRow = (selectedRow - 1 + ROW_COUNT) % ROW_COUNT;
            break;
        case SDLK_DOWN:
        case SDLK_s:
            selectedRow = (selectedRow + 1) % ROW_COUNT;
            break;

        case SDLK_LEFT:
        case SDLK_a:
            if (selectedRow == ROW_MUSIC)
                config.musicVolume = std::max(0, config.musicVolume - 5);
            else if (selectedRow == ROW_SFX)
                config.sfxVolume = std::max(0, config.sfxVolume - 5);
            applyConfig();
            break;

        case SDLK_RIGHT:
        case SDLK_d:
            if (selectedRow == ROW_MUSIC)
                config.musicVolume = std::min(100, config.musicVolume + 5);
            else if (selectedRow == ROW_SFX)
                config.sfxVolume = std::min(100, config.sfxVolume + 5);
            applyConfig();
            break;

        case SDLK_RETURN:
        case SDLK_KP_ENTER:
        case SDLK_SPACE:
            if (selectedRow == ROW_FULLSCR)
            {
                config.fullscreen = !config.fullscreen;
                applyConfig();
            }
            else if (selectedRow == ROW_SAVE)
            {
                config.save();
                applyConfig();
                out = ScreenResult::GO_MAIN_MENU;
                return true;
            }
            else if (selectedRow == ROW_CANCEL)
            {
                config = original;
                applyConfig();
                out = ScreenResult::GO_MAIN_MENU;
                return true;
            }
            break;

        default:
            break;
        }
    }

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        int mx = e.button.x, my = e.button.y;

        const int labelW = 180;
        const int sliderX = panel.x + labelW + 10;
        const int sliderW = panel.w - labelW - 30;

        for (int row = 0; row < ROW_COUNT; ++row)
        {
            int ry = optStartY + row * (ROW_H + ROW_GAP);
            SDL_Rect rowRect = {panel.x + 10, ry, panel.w - 20, ROW_H};
            if (!(mx >= rowRect.x && mx <= rowRect.x + rowRect.w &&
                  my >= rowRect.y && my <= rowRect.y + rowRect.h))
                continue;

            selectedRow = row;

            if (row == ROW_MUSIC || row == ROW_SFX)
            {
                if (mx >= sliderX && mx <= sliderX + sliderW)
                {
                    int val = static_cast<int>((float)(mx - sliderX) / sliderW * 100);
                    val = std::clamp(val, 0, 100);
                    if (row == ROW_MUSIC)
                        config.musicVolume = val;
                    else
                        config.sfxVolume = val;
                    applyConfig();
                }
            }
            else if (row == ROW_FULLSCR)
            {
                config.fullscreen = !config.fullscreen;
                applyConfig();
            }
        }

        // Botones de guardar/cancelar
        const int btnW = (panel.w - 30) / 2;
        const int btnY = panel.y + panel.h - 50;
        SDL_Rect bSave = {panel.x + 10, btnY, btnW, 38};
        SDL_Rect bCancel = {panel.x + 20 + btnW, btnY, btnW, 38};

        if (mx >= bSave.x && mx <= bSave.x + bSave.w && my >= bSave.y && my <= bSave.y + bSave.h)
        {
            config.save();
            applyConfig();
            out = ScreenResult::GO_MAIN_MENU;
            return true;
        }
        if (mx >= bCancel.x && mx <= bCancel.x + bCancel.w && my >= bCancel.y && my <= bCancel.y + bCancel.h)
        {
            config = original;
            applyConfig();
            out = ScreenResult::GO_MAIN_MENU;
            return true;
        }
    }

    // Drag sobre sliders
    if (e.type == SDL_MOUSEMOTION && (e.motion.state & SDL_BUTTON_LMASK))
    {
        int mx = e.motion.x;
        const int labelW = 180;
        const int sliderX = panel.x + labelW + 10;
        const int sliderW = panel.w - labelW - 30;

        if (selectedRow == ROW_MUSIC || selectedRow == ROW_SFX)
        {
            if (mx >= sliderX && mx <= sliderX + sliderW)
            {
                int val = static_cast<int>((float)(mx - sliderX) / sliderW * 100);
                val = std::clamp(val, 0, 100);
                if (selectedRow == ROW_MUSIC)
                    config.musicVolume = val;
                else
                    config.sfxVolume = val;
                applyConfig();
            }
        }
    }

    return false;
}

void ConfigScreen::applyConfig()
{
    // Volumen de musica (SDL_mixer)
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == 0 || Mix_QuerySpec(nullptr, nullptr, nullptr))
    {
        Mix_VolumeMusic(config.musicVolume * MIX_MAX_VOLUME / 100);
        Mix_Volume(-1, config.sfxVolume * MIX_MAX_VOLUME / 100);
    }

    // Fullscreen
    Uint32 flags = config.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
    SDL_SetWindowFullscreen(window, flags);

    SDL_GetWindowSize(window, &windowW, &windowH);
    computeLayout();
}

// Renderizado

void ConfigScreen::render()
{
    renderBackground();
    renderPanel();
    renderHeader();
    renderOptions();
    renderFooter();
}

void ConfigScreen::renderBackground()
{
    SDL_SetRenderDrawColor(renderer, C_BG.r, C_BG.g, C_BG.b, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 40, 30, 80, 30);
    for (int y = 0; y < windowH; y += 40)
        SDL_RenderDrawLine(renderer, 0, y, windowW, y);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void ConfigScreen::renderPanel()
{
    SDL_Rect shadow = {panel.x + 6, panel.y + 6, panel.w, panel.h};
    drawRect(shadow, {0, 0, 0, 120}, true);
    drawFilledRoundRect(panel, C_PANEL);
    drawBorderRoundRect(panel, C_PANEL_BORD);
    SDL_Rect div = {panel.x + 14, panel.y + 72, panel.w - 28, 1};
    drawRect(div, C_DIVIDER, true);
}

void ConfigScreen::renderHeader()
{
    SDL_Rect tr{};
    SDL_Texture *t = makeText("Configuración", fontTitle, C_TITLE, tr);
    if (t)
    {
        tr.x = panel.x + (panel.w - tr.w) / 2;
        tr.y = panel.y + 18;
        drawTex(t, tr.x, tr.y);
        SDL_DestroyTexture(t);
    }
    SDL_Rect sr{};
    SDL_Texture *s = makeText("↑↓ navegar   ←→ ajustar   Enter confirmar   Esc cancelar",
                              fontSmall, C_DIM, sr);
    if (s)
    {
        sr.x = panel.x + (panel.w - sr.w) / 2;
        sr.y = panel.y + 55;
        drawTex(s, sr.x, sr.y);
        SDL_DestroyTexture(s);
    }
}

void ConfigScreen::renderOptions()
{
    const int labelW = 180;
    const int sliderX = panel.x + labelW + 10;
    const int sliderW = panel.w - labelW - 30;

    auto rowLabel = [&](int row, const char *label)
    {
        bool sel = (selectedRow == row);
        SDL_Color c = sel ? C_SEL : C_TEXT;
        int ry = optStartY + row * (ROW_H + ROW_GAP);

        if (sel)
        {
            SDL_Rect bg = {panel.x + 8, ry, panel.w - 16, ROW_H};
            drawFilledRoundRect(bg, C_SEL_BG, 4);
            drawBorderRoundRect(bg, C_SEL, 4);
            SDL_Rect bar = {panel.x + 8, ry + 8, 3, ROW_H - 16};
            drawRect(bar, C_SEL, true);
        }

        SDL_Rect lr{};
        SDL_Texture *lt = makeText(label, fontMedium, c, lr);
        if (lt)
        {
            lr.x = panel.x + 18;
            lr.y = ry + (ROW_H - lr.h) / 2;
            drawTex(lt, lr.x, lr.y);
            SDL_DestroyTexture(lt);
        }
        return ry;
    };

    // Volumen musica
    {
        int ry = rowLabel(ROW_MUSIC, "Música");
        drawSlider(sliderX, ry + (ROW_H - 16) / 2, sliderW,
                   config.musicVolume, selectedRow == ROW_MUSIC, false);
        SDL_Rect vr{};
        SDL_Texture *vt = makeText(std::to_string(config.musicVolume), fontSmall, C_DIM, vr);
        if (vt)
        {
            vr.x = sliderX + sliderW + 8;
            vr.y = ry + (ROW_H - vr.h) / 2;
            drawTex(vt, vr.x, vr.y);
            SDL_DestroyTexture(vt);
        }
    }

    // Volumen efectos
    {
        int ry = rowLabel(ROW_SFX, "Efectos");
        drawSlider(sliderX, ry + (ROW_H - 16) / 2, sliderW,
                   config.sfxVolume, selectedRow == ROW_SFX, false);
        SDL_Rect vr{};
        SDL_Texture *vt = makeText(std::to_string(config.sfxVolume), fontSmall, C_DIM, vr);
        if (vt)
        {
            vr.x = sliderX + sliderW + 8;
            vr.y = ry + (ROW_H - vr.h) / 2;
            drawTex(vt, vr.x, vr.y);
            SDL_DestroyTexture(vt);
        }
    }

    // Pantalla completa
    {
        int ry = rowLabel(ROW_FULLSCR, "Pantalla completa");
        drawToggle(sliderX, ry + (ROW_H - 26) / 2, config.fullscreen,
                   selectedRow == ROW_FULLSCR, false);
    }
}

void ConfigScreen::renderFooter()
{
    const int btnW = (panel.w - 30) / 2;
    const int btnY = panel.y + panel.h - 50;

    SDL_Rect divider = {panel.x + 14, btnY - 12, panel.w - 28, 1};
    drawRect(divider, C_DIVIDER, true);

    SDL_Rect bSave = {panel.x + 10, btnY, btnW, 38};
    SDL_Rect bCancel = {panel.x + 20 + btnW, btnY, btnW, 38};

    // Boton guardar
    SDL_Color saveBg = hoveredSave
                           ? SDL_Color{50, 130, 70, 210}
                           : C_BTN_SAVE;
    drawFilledRoundRect(bSave, saveBg, 5);
    drawBorderRoundRect(bSave, hoveredSave ? C_HOV : C_ON, 5);
    {
        SDL_Rect tr{};
        SDL_Texture *t = makeText("Guardar", fontMedium,
                                  hoveredSave ? C_HOV : C_TEXT, tr);
        if (t)
        {
            tr.x = bSave.x + (bSave.w - tr.w) / 2;
            tr.y = bSave.y + (bSave.h - tr.h) / 2;
            drawTex(t, tr.x, tr.y);
            SDL_DestroyTexture(t);
        }
    }

    // Boton cancelar
    SDL_Color cancBg = hoveredCancel
                           ? SDL_Color{130, 45, 45, 210}
                           : C_BTN_CANC;
    drawFilledRoundRect(bCancel, cancBg, 5);
    drawBorderRoundRect(bCancel, hoveredCancel ? C_HOV : C_OFF, 5);
    {
        SDL_Rect tr{};
        SDL_Texture *t = makeText("Cancelar", fontMedium,
                                  hoveredCancel ? C_HOV : C_TEXT, tr);
        if (t)
        {
            tr.x = bCancel.x + (bCancel.w - tr.w) / 2;
            tr.y = bCancel.y + (bCancel.h - tr.h) / 2;
            drawTex(t, tr.x, tr.y);
            SDL_DestroyTexture(t);
        }
    }
}

// Widgets

void ConfigScreen::drawSlider(int x, int y, int w, int value, bool selected, bool /*hovered*/)
{
    // Fondo
    SDL_Rect bg = {x, y, w, 16};
    drawFilledRoundRect(bg, C_SLIDER_BG, 8);

    // Fill
    int fillW = static_cast<int>((float)value / 100.f * w);
    if (fillW > 0)
    {
        SDL_Rect fill = {x, y, fillW, 16};
        drawFilledRoundRect(fill, selected ? C_SLIDER_SEL : C_SLIDER_FG, 8);
    }

    // Thumb
    int tx = x + fillW - 6;
    tx = std::clamp(tx, x, x + w - 12);
    SDL_Rect thumb = {tx, y - 3, 12, 22};
    drawFilledRoundRect(thumb, selected ? C_HOV : C_TEXT, 4);
    drawBorderRoundRect(thumb, selected ? C_SEL : C_DIM, 4);
}

void ConfigScreen::drawToggle(int x, int y, bool value, bool selected, bool /*hovered*/)
{
    const int W = 52, H = 26;
    SDL_Rect bg = {x, y, W, H};
    SDL_Color bgColor = value ? C_ON : C_OFF;
    drawFilledRoundRect(bg, bgColor, H / 2);
    drawBorderRoundRect(bg, selected ? C_SEL : bgColor, H / 2);

    // Círculo
    int cx = value ? x + W - H + 3 : x + 3;
    SDL_Rect circle = {cx, y + 3, H - 6, H - 6};
    drawFilledRoundRect(circle, C_TEXT, (H - 6) / 2);

    // Label ON/OFF
    SDL_Rect lr{};
    SDL_Texture *lt = makeText(value ? "ON" : "OFF", fontSmall, C_TEXT, lr);
    if (lt)
    {
        lr.x = x + W + 12;
        lr.y = y + (H - lr.h) / 2;
        drawTex(lt, lr.x, lr.y);
        SDL_DestroyTexture(lt);
    }
}

// Helpers de dibujo

void ConfigScreen::drawFilledRoundRect(const SDL_Rect &r, SDL_Color color, int radius)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect center = {r.x + radius, r.y, r.w - radius * 2, r.h};
    SDL_RenderFillRect(renderer, &center);
    SDL_Rect left = {r.x, r.y + radius, radius, r.h - radius * 2};
    SDL_Rect right = {r.x + r.w - radius, r.y + radius, radius, r.h - radius * 2};
    SDL_RenderFillRect(renderer, &left);
    SDL_RenderFillRect(renderer, &right);
    for (int dy = 0; dy < radius; dy++)
    {
        int dx = static_cast<int>(std::sqrt(static_cast<float>(radius * radius - dy * dy)));
        SDL_RenderDrawLine(renderer, r.x + radius - dx, r.y + dy, r.x + radius, r.y + dy);
        SDL_RenderDrawLine(renderer, r.x + r.w - radius, r.y + dy, r.x + r.w - radius + dx - 1, r.y + dy);
        SDL_RenderDrawLine(renderer, r.x + radius - dx, r.y + r.h - 1 - dy, r.x + radius, r.y + r.h - 1 - dy);
        SDL_RenderDrawLine(renderer, r.x + r.w - radius, r.y + r.h - 1 - dy, r.x + r.w - radius + dx - 1, r.y + r.h - 1 - dy);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void ConfigScreen::drawBorderRoundRect(const SDL_Rect &r, SDL_Color color, int radius)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLine(renderer, r.x + radius, r.y, r.x + r.w - radius, r.y);
    SDL_RenderDrawLine(renderer, r.x + radius, r.y + r.h - 1, r.x + r.w - radius, r.y + r.h - 1);
    SDL_RenderDrawLine(renderer, r.x, r.y + radius, r.x, r.y + r.h - radius);
    SDL_RenderDrawLine(renderer, r.x + r.w - 1, r.y + radius, r.x + r.w - 1, r.y + r.h - radius);
    for (int dy = 0; dy < radius; dy++)
    {
        int dx = static_cast<int>(std::sqrt(static_cast<float>(radius * radius - dy * dy)));
        SDL_RenderDrawPoint(renderer, r.x + radius - dx, r.y + dy);
        SDL_RenderDrawPoint(renderer, r.x + r.w - radius + dx - 1, r.y + dy);
        SDL_RenderDrawPoint(renderer, r.x + radius - dx, r.y + r.h - 1 - dy);
        SDL_RenderDrawPoint(renderer, r.x + r.w - radius + dx - 1, r.y + r.h - 1 - dy);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

SDL_Texture *ConfigScreen::makeText(const std::string &text, TTF_Font *font,
                                    SDL_Color color, SDL_Rect &out)
{
    if (text.empty())
        return nullptr;
    SDL_Surface *s = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!s)
        return nullptr;
    out.w = s->w;
    out.h = s->h;
    SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, s);
    SDL_FreeSurface(s);
    return t;
}

void ConfigScreen::drawTex(SDL_Texture *tex, int x, int y)
{
    if (!tex)
        return;
    int w, h;
    SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
    SDL_Rect dst = {x, y, w, h};
    SDL_RenderCopy(renderer, tex, nullptr, &dst);
}

void ConfigScreen::drawRect(const SDL_Rect &r, SDL_Color color, bool fill)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    if (fill)
        SDL_RenderFillRect(renderer, &r);
    else
        SDL_RenderDrawRect(renderer, &r);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}
