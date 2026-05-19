
#ifndef PRUEBA_SDL_ANIMATION_H
#define PRUEBA_SDL_ANIMATION_H


struct Animation {

    int index;
    int frames;
    int speed;

    Animation(){}
    Animation(int i,int f,int s) {
        this->index = i;
        this->frames = f;
        this->speed = s;
    }
};



#endif //PRUEBA_SDL_ANIMATION_H
