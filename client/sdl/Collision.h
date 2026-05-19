
#ifndef PRUEBA_SDL_COLLISION_H
#define PRUEBA_SDL_COLLISION_H
#include "cmake-build-debug/_deps/sdl2-src/include/SDL_rect.h"

class ColliderComponent;
class Collision {
public:
    static bool AABB(const SDL_Rect& rectA, const SDL_Rect& rectB);
    static bool AABB(const ColliderComponent& colA,const ColliderComponent& colB);
};
#endif //PRUEBA_SDL_COLLISION_H
