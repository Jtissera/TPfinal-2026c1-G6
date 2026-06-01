#pragma once

#include <cstdint>
#include <string>

enum class NpcType : uint8_t
{
  NONE = 0,

  // NPCs de ciudad
  PRIEST = 1,
  MERCHANT = 2,
  BANKER = 3,

  // Zona de combate (mundo principal)
  GOBLIN = 10,
  SKELETON = 11,
  ZOMBIE = 12,
  GUARD = 13,

  // Caverna
  GOBLIN_CAVE = 20,
  SKELETON_CAVE = 21,
  SPIDER_CAVE = 22,
  GOLEM_CAVE = 23,

  // Mazmorra
  GOBLIN_DUNGEON = 30,
  SKELETON_DUNGEON = 31,
  SPIDER_DUNGEON = 32,
  GOLEM_DUNGEON = 33,

  // Desierto (zona de combate exterior)
  GOBLIN_DESERT = 40,
  SKELETON_DESERT = 41,
  SPIDER_DESERT = 42,
  GOLEM_DESERT = 43,
};

inline std::string npcTypeName(NpcType type)
{
  switch (type)
  {
  case NpcType::NONE:
    return "Ninguno";
  case NpcType::PRIEST:
    return "Sacerdote";
  case NpcType::MERCHANT:
    return "Comerciante";
  case NpcType::BANKER:
    return "Banquero";
  case NpcType::GOBLIN:
    return "Goblin";
  case NpcType::SKELETON:
    return "Esqueleto";
  case NpcType::ZOMBIE:
    return "Zombie";
  case NpcType::GUARD:
    return "Guardia";
  case NpcType::GOBLIN_CAVE:
    return "Goblin de Caverna";
  case NpcType::SKELETON_CAVE:
    return "Esqueleto de Caverna";
  case NpcType::SPIDER_CAVE:
    return "Araña de Caverna";
  case NpcType::GOLEM_CAVE:
    return "Golem de Caverna";
  case NpcType::GOBLIN_DUNGEON:
    return "Goblin de Mazmorra";
  case NpcType::SKELETON_DUNGEON:
    return "Esqueleto de Mazmorra";
  case NpcType::SPIDER_DUNGEON:
    return "Araña de Mazmorra";
  case NpcType::GOLEM_DUNGEON:
    return "Golem de Mazmorra";
  case NpcType::GOBLIN_DESERT:
    return "Goblin del Desierto";
  case NpcType::SKELETON_DESERT:
    return "Esqueleto del Desierto";
  case NpcType::SPIDER_DESERT:
    return "Araña del Desierto";
  case NpcType::GOLEM_DESERT:
    return "Golem del Desierto";
  default:
    return "Desconocido";
  }
}

inline std::string npcTypeKey(NpcType type)
{
  switch (type)
  {
  case NpcType::GOBLIN:
    return "goblin";
  case NpcType::SKELETON:
    return "skeleton";
  case NpcType::ZOMBIE:
    return "zombie";
  case NpcType::GUARD:
    return "guard";
  case NpcType::PRIEST:
    return "priest";
  case NpcType::MERCHANT:
    return "merchant";
  case NpcType::BANKER:
    return "banker";
  case NpcType::GOBLIN_CAVE:
    return "goblin_cave";
  case NpcType::SKELETON_CAVE:
    return "skeleton_cave";
  case NpcType::SPIDER_CAVE:
    return "spider_cave";
  case NpcType::GOLEM_CAVE:
    return "golem_cave";
  case NpcType::GOBLIN_DUNGEON:
    return "goblin_dungeon";
  case NpcType::SKELETON_DUNGEON:
    return "skeleton_dungeon";
  case NpcType::SPIDER_DUNGEON:
    return "spider_dungeon";
  case NpcType::GOLEM_DUNGEON:
    return "golem_dungeon";
  case NpcType::GOBLIN_DESERT:
    return "goblin_desert";
  case NpcType::SKELETON_DESERT:
    return "skeleton_desert";
  case NpcType::SPIDER_DESERT:
    return "spider_desert";
  case NpcType::GOLEM_DESERT:
    return "golem_desert";
  default:
    return "";
  }
}

inline bool isSpawnable(NpcType type)
{
  switch (type)
  {
  case NpcType::PRIEST:
  case NpcType::MERCHANT:
  case NpcType::BANKER:
    return false;
  default:
    return type != NpcType::NONE && !npcTypeKey(type).empty();
  }
}