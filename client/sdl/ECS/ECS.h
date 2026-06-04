
#ifndef PRUEBA_SDL_ECS_H
#define PRUEBA_SDL_ECS_H
#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <bitset>
#include  <array>

#include "client/sdl/RenderContext.h"
#include "client/sdl/UpdateContext.h"


class Component;
class Entity;
class Manager;

using ComponentID = std::size_t;
using Group = std::size_t;
constexpr std::size_t maxComponents = 32;
constexpr std::size_t maxGroups = 32;

// genera id unicos para cada componente nueva
inline ComponentID getNewComponentTypeID() {
    static ComponentID lastID = 0u;
    return lastID++;
}

template <typename T> inline ComponentID getComponentTypeID() noexcept {
    static_assert(std::is_base_of<Component, T >::value,"");
    static ComponentID typeID = getNewComponentTypeID(); // Este static se crea una sola vez por cada tipo T.
    return typeID;
} // devuelve siempre el mismo id para las componentes, ej: transformcomponent



using  ComponentBitSet = std::bitset<maxComponents>; // sirve para saber si una entidad tiene cierto componente
using  GroupBitSet = std::bitset<maxGroups>; // idem pero para grupos
using  ComponentArray = std::array<Component*,maxComponents>; //sirve paara acceder rapidamente al componente

// clase base de cada componente
class Component {

public:
    Entity* entity = nullptr;
    virtual void init(){};
    virtual void update(UpdateContext&) {}
    virtual void draw(RenderContext&) {}
    virtual ~Component() = default;
};


class Entity {

private:
    Manager& manager;  // referencia al manager dueño
    bool active = true;  // si esta vida o destruida
    std::vector<std::unique_ptr<Component>> components; // dueña de las comp

    ComponentArray componentArray;  // acceso rapido por tipo
    ComponentBitSet componentBitSet;  // los componentes que tiene
    GroupBitSet groupBitSet;  // dice el grupo al que pertenece

public:
    explicit Entity(Manager& mManager): manager(mManager){}

    // para cada entidad recorre sus componentes y que se actualice cada una
    void update(UpdateContext& context) {
        for (auto& c : components) {
            c->update(context);
        }
    }

    void draw(RenderContext& context) {
        for (auto& c : components) {
            c->draw(context);
        }
    }

    bool isActive() const {return active;}
    void destroy(){active = false;}

    template <typename T> bool hasComponent()const {
        return componentBitSet[getComponentTypeID<T>()];
    }
    bool hasGroup(Group mGroup) {
        return groupBitSet[mGroup];
    }

    void addGroup(Group mGroup);
    void delGroup(Group mGroup) {

        groupBitSet[mGroup] = false;
    }

    template <typename T, typename... TArgs>
    T& addComponent(TArgs&&... mArgs) {

        T* c(new T(std::forward<TArgs>(mArgs)...));
        c->entity = this;
        std::unique_ptr<Component> uPtr{c};
        components.emplace_back(std::move(uPtr));

        componentArray[getComponentTypeID<T>()] = c;
        componentBitSet[getComponentTypeID<T>()] = true;
        c->init();
        return  *c;
    }

    template<typename T> T& getComponent() const {
        auto ptr(componentArray[getComponentTypeID<T>()]);
        return *static_cast<T*>(ptr);

    }
};

class Manager {

private:
    std::vector<std::unique_ptr<Entity>> entities;
    std::array<std::vector<Entity*>,maxGroups> groupedEntities;
public:
    void update(UpdateContext& context) {
        for (auto& e : entities) {
            e->update(context);
        }
    }

    void draw(RenderContext& context) {
        for (auto& e : entities) {
            e->draw(context);
        }
    }

    void refresh() {
        for (auto i(0u);i < maxGroups;i++) {
            auto& v(groupedEntities[i]);
            v.erase(std::remove_if(std::begin(v),std::end(v),
                [i](Entity* mEntity){
                return !mEntity->isActive()|| !mEntity->hasGroup(i);
            }),
            std::end(v));
        }

        entities.erase(std::remove_if(std::begin(entities),std::end(entities),
            [](const std::unique_ptr<Entity>&mEntity) {
                return !mEntity->isActive();
            }),std::end(entities));
    }
    void AddToGroup(Entity* mEntity,Group mGroup) {
        groupedEntities[mGroup].emplace_back(mEntity);
    }

    std::vector<Entity*>& getGroup(Group mGroup) {
        return groupedEntities[mGroup];
    }

    Entity& addEntity() {
        Entity* e = new Entity(*this);
        std::unique_ptr<Entity> uPtr{e};
        entities.emplace_back(std::move(uPtr));
        return *e;
    }
};
#endif //PRUEBA_SDL_ECS_H
