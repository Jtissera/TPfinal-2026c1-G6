#ifndef PRUEBA_SDL_ANIMATION_H
#define PRUEBA_SDL_ANIMATION_H

struct Animation {
    int index;
    int frames;
    int speed;
    int startFrame;
    int startX;

    Animation() : index(0), frames(0), speed(0), startFrame(0), startX(-1) {}

    Animation(int i, int f, int s, int startFrame = 0, int startX = -1)
        : index(i), frames(f), speed(s), startFrame(startFrame), startX(startX) {}
};

#endif //PRUEBA_SDL_ANIMATION_H