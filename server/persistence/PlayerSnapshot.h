#pragma once
#include <cstdint>
#include <cstring>

// TOML

static constexpr uint8_t SNAPSHOT_VERSION = 1;
static constexpr uint32_t MAX_NAME_LEN = 32;
static constexpr uint32_t MAX_MAP_ID_LEN = 64;
static constexpr uint32_t MAX_RACE_LEN = 32;
static constexpr uint32_t MAX_CLASS_LEN = 32;
static constexpr uint32_t MAX_ITEMS = 20;

struct ItemSnapshot {
  uint32_t catalogId = 0;
  uint8_t slot = 0;
  uint8_t effect = 0;
  uint16_t damageMin = 0;
  uint16_t damageMax = 0;
  uint16_t defenseMin = 0;
  uint16_t defenseMax = 0;
  uint16_t healAmount = 0;
  uint16_t manaAmount = 0;
  uint16_t manaCost = 0;
  uint8_t isRanged = 0;
  uint8_t _pad[2] = {}; // me evita Syscall param write(buf) points to
                        // uninitialised byte(s) de valgrind
};
static_assert(sizeof(ItemSnapshot) == 24, "ItemSnapshot size mismatch");

struct EquipSlotSnapshot {
  uint32_t catalogId = 0;
  uint8_t slot = 0;
  uint8_t _pad[3] = {};
};
static_assert(sizeof(EquipSlotSnapshot) == 8,
              "EquipSlotSnapshot size mismatch");

struct PlayerSnapshot {
  uint8_t version = SNAPSHOT_VERSION;
  uint8_t _pad0[3] = {};

  char name[MAX_NAME_LEN] = {};
  char mapId[MAX_MAP_ID_LEN] = {};
  char race[MAX_RACE_LEN] = {};
  char cls[MAX_CLASS_LEN] = {};

  float pixelX = 0.f;
  float pixelY = 0.f;

  int16_t hp = 0;
  int16_t maxHp = 0;
  int16_t mana = 0;
  int16_t maxMana = 0;

  uint8_t level = 1;
  uint8_t _pad1 = {};

  uint32_t gold = 0;
  uint32_t experience = 0;

  uint32_t gameId = 0;
  // Inventario (slots de mochila)
  uint8_t itemCount = 0;
  uint8_t _pad2[3] = {};
  ItemSnapshot items[MAX_ITEMS] = {};

  // Equipo equipado (un slot por tipo)
  EquipSlotSnapshot equipped[4] = {};

  uint8_t _reserved[64] =
      {}; // espacio para futuras versiones (clanes probablemente)
};
static_assert(std::is_trivially_copyable_v<PlayerSnapshot>,
              "must be POD"); // en caso de que no se
                              // traivialmente copiable esto no
                              // deja compilar, es un seguro contra
                              // metodos dinamicos o string o
                              // cualquier cosa que implique un puntero