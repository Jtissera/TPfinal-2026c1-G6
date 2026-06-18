#include "MiniChat.h"
#include <algorithm>

SDL_Color MiniChat::colorFor(ChatMsgType type)
{
    switch (type)
    {
    case ChatMsgType::GENERAL:
        return {255, 255, 255, 255};
    case ChatMsgType::PRIVATE:
        return {255, 220, 60, 255};
    case ChatMsgType::DAMAGE_DEALT:
        return {220, 60, 60, 255};
    case ChatMsgType::DAMAGE_TAKEN:
        return {80, 200, 80, 255};
    case ChatMsgType::INFO:
        return {100, 200, 255, 255};
    case ChatMsgType::CLAN:
        return {0, 255, 128, 255};
    }
    return {255, 255, 255, 255};
}

void MiniChat::appendLine(const std::string &text, ChatMsgType type)
{
    lines.push_back({text, type});

    if (static_cast<int>(lines.size()) > HISTORY_MAX_LINES)
        lines.pop_front();

    scrollOffset = 0;
}

void MiniChat::setFocused(bool f)
{
    focused = f;
    if (f)
        SDL_StartTextInput();
    else
        SDL_StopTextInput();
}

bool MiniChat::isFocused() const { return focused; }
bool MiniChat::hasPendingInput() const { return pendingInput; }

std::string MiniChat::consumeInput()
{
    pendingInput = false;
    std::string out = std::move(pendingText);
    pendingText.clear();
    return out;
}

bool MiniChat::handleEvent(const SDL_Event &event)
{
    if (event.type == SDL_MOUSEBUTTONDOWN &&
        event.button.button == SDL_BUTTON_LEFT)
    {
        const int mx = event.button.x;
        const int my = event.button.y;
        const bool insideInput = mx >= INPUT_X && mx < INPUT_X + INPUT_W &&
                                 my >= INPUT_Y && my < INPUT_Y + INPUT_H;
        if (insideInput)
        {
            setFocused(true);
            return true;
        }

        if (focused)
            setFocused(false);
        return false;
    }

    if (event.type == SDL_MOUSEWHEEL)
    {
        int mx, my;
        SDL_GetMouseState(&mx, &my);

        if (mx >= 0 && mx <= 900 && my >= 33 && my <= 133)
        {
            if (event.wheel.y > 0)
            {
                scrollOffset++;
            }
            else if (event.wheel.y < 0)
            {
                scrollOffset--;
            }

            int maxScroll = std::max(0, static_cast<int>(lines.size()) - CHAT_MAX_LINES);
            if (scrollOffset > maxScroll)
                scrollOffset = maxScroll;
            if (scrollOffset < 0)
                scrollOffset = 0;

            return true;
        }
    }

    if (!focused)
        return false;

    if (event.type == SDL_TEXTINPUT)
    {
        if (inputBuffer.size() < CHAT_MAX_INPUT)
            inputBuffer += event.text.text;
        return true;
    }

    if (event.type == SDL_KEYDOWN)
    {
        switch (event.key.keysym.sym)
        {
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            if (!inputBuffer.empty())
            {
                pendingText = inputBuffer;
                pendingInput = true;
                inputBuffer.clear();
            }
            return true;
        case SDLK_BACKSPACE:
            if (!inputBuffer.empty())
                inputBuffer.pop_back();
            return true;
        case SDLK_ESCAPE:
            inputBuffer.clear();
            setFocused(false);
            return true;
        default:

            return true;
        }
    }

    if (event.type == SDL_KEYUP && focused)
        return true;

    return false;
}

void MiniChat::renderText(SDL_Renderer *renderer, TTF_Font *font,
                          const std::string &text, SDL_Color color,
                          int x, int y, int maxW) const
{
    if (!font || text.empty())
        return;

    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surf)
        return;

    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (!tex)
    {
        SDL_FreeSurface(surf);
        return;
    }

    const int w = std::min(surf->w, maxW);
    SDL_Rect src = {0, 0, w, surf->h};
    SDL_Rect dest = {x, y, w, surf->h};
    SDL_RenderCopy(renderer, tex, &src, &dest);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

void MiniChat::render(SDL_Renderer *renderer, TTF_Font *font) const
{
    const int totalLines = static_cast<int>(lines.size());
    const int visibleCount = std::min(totalLines, CHAT_MAX_LINES);

    int startIndex = totalLines - CHAT_MAX_LINES - scrollOffset;
    if (startIndex < 0)
        startIndex = 0;

    for (int i = 0; i < visibleCount; ++i)
    {
        int lineIndex = startIndex + i;
        renderText(renderer, font, lines[lineIndex].text, colorFor(lines[lineIndex].type),
                   LOG_X, LOG_Y + i * LINE_H, LOG_W);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 140);
    SDL_Rect inputRect = {INPUT_X, INPUT_Y, INPUT_W, INPUT_H};
    SDL_RenderFillRect(renderer, &inputRect);

    SDL_SetRenderDrawColor(renderer, focused ? 180 : 80,
                           focused ? 140 : 80,
                           focused ? 40 : 80,
                           focused ? 255 : 200);
    SDL_RenderDrawRect(renderer, &inputRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    const std::string display = inputBuffer +
                                (focused && (SDL_GetTicks() / 500) % 2 == 0 ? "|" : "");

    SDL_Color white = {255, 255, 255, 255};
    renderText(renderer, font, display, white,
               INPUT_X + 4, INPUT_Y + 3, INPUT_W - 8);
}