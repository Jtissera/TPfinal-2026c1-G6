
#ifndef TALLER_TP_UPDATECONTEXT_H
#define TALLER_TP_UPDATECONTEXT_H

#pragma once

#include <SDL2/SDL.h>
#include <memory>

#include "common/queue.h"
#include "common/network/messages/message.h"

struct UpdateContext {
    const Uint8* keyboardState = nullptr;
    Queue<std::shared_ptr<const Message>>* sendQueue = nullptr;
    SDL_Rect camera;
};

#endif //TALLER_TP_UPDATECONTEXT_H
