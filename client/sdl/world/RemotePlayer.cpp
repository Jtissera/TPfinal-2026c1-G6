#include "RemotePlayer.h"

#include "common/dtos/gameTypes.h"

RemotePlayer::RemotePlayer(uint32_t id, Entity *entity, const PlayerDto &dto)
    : id(id),
      entity(entity), dto(dto)
{
}

uint32_t RemotePlayer::getId() const
{
    return id;
}

Entity *RemotePlayer::getEntity()
{
    return entity;
}

const Entity *RemotePlayer::getEntity() const
{
    return entity;
}

void RemotePlayer::setPositionAndAnimation(
    float x,
    float y,
    Direction direction,
    bool moving)
{
    // Si no hay entidad asociada, no podemos actualizar nada.
    if (entity == nullptr)
    {
        return;
    }

    std::cout << "[REMOTE SYNC] id=" << id << " pos=(" << x << ", " << y << ")" << " ghost=" << ghost << " moving=" << moving << std::endl;

    // Actualizamos la posición visual del jugador remoto.
    auto &transform = entity->getComponent<TransformComponent>();
    transform.position.x = x;
    transform.position.y = y;

    // Si no tiene sprite, no hay animación que actualizar.
    if (!entity->hasComponent<SpriteComponent>())
    {
        return;
    }

    // Elegimos una animación según dirección y movimiento.
    // Funciona para vivos y fantasmas mientras compartan nombres:
    // IdleDown, IdleUp, WalkDown, etc.
    auto &sprite = entity->getComponent<SpriteComponent>();
    const std::string animationName = animationNameFor(direction, moving);
    sprite.Play(animationName.c_str());
}

void RemotePlayer::setEquipment(
    const EquipmentDto &equipment,
    const ItemCatalog &itemCatalog)
{
    if (entity == nullptr)
    {
        return;
    }

    if (!entity->hasComponent<EquipmentComponent>())
    {
        std::cerr << "[REMOTE EQUIP] remote entity no tiene EquipmentComponent. id="
                  << id
                  << std::endl;
        return;
    }

    auto &component = entity->getComponent<EquipmentComponent>();
    component.setFromDto(equipment, itemCatalog);

    std::cout << "[REMOTE EQUIP] id="
              << id
              << " weaponCatalogId=" << equipment.weaponCatalogId
              << " armorCatalogId=" << equipment.armorCatalogId
              << " helmetCatalogId=" << equipment.helmetCatalogId
              << " shieldCatalogId=" << equipment.shieldCatalogId
              << std::endl;
}

bool RemotePlayer::isGhost() const
{
    return ghost;
}

void RemotePlayer::setGhost(bool value)
{
    ghost = value;
}

const char *RemotePlayer::directionSuffix(Direction direction) const
{
    // Traducimos la dirección del protocolo al nombre usado por las animaciones.
    switch (direction)
    {
    case Direction::UP:
        return "Up";

    case Direction::DOWN:
        return "Down";

    case Direction::LEFT:
        return "Left";

    case Direction::RIGHT:
        return "Right";

    case Direction::NONE:
    default:
        // Por convención, si no hay dirección miramos hacia abajo.
        return "Down";
    }
}

std::string RemotePlayer::animationNameFor(Direction direction, bool moving) const
{
    // Si el fantasma no tiene animación de caminata, podés cambiar esto a:
    // const char* prefix = ghost ? "Idle" : (moving ? "Walk" : "Idle");
    const char *prefix = moving ? "Walk" : "Idle";

    // Armamos nombres como "WalkDown", "IdleLeft", etc.
    return std::string(prefix) + directionSuffix(direction);
}

const PlayerDto &RemotePlayer::getDto() const
{
    return dto;
}

void RemotePlayer::updateDto(const PlayerDto &newDto)
{
    dto = newDto;
}

uint8_t RemotePlayer::getLevel() const
{
    return dto.level;
}

void RemotePlayer::setLevel(uint8_t newLevel)
{
    dto.level = newLevel;
}

void RemotePlayer::setLevel(uint32_t newLevel)
{
    // Si no hay entidad asociada, no podemos actualizar nada.
    if (entity == nullptr)
    {
        return;
    }

    // Si la entidad tiene NameplateComponent, actualizamos el nivel visual.
    if (entity->hasComponent<NameplateComponent>())
    {
        entity->getComponent<NameplateComponent>().setLevel(newLevel);
    }
}

void RemotePlayer::setClan(const std::string& newClan)
{
    if (entity == nullptr)
    {
        return;
    }
    if (entity->hasComponent<NameplateComponent>())
    {
        entity->getComponent<NameplateComponent>().setClan(newClan);
    }
}