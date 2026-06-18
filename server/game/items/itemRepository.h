#pragma once
#include "item.h"
#include <optional>
#include <string>
#include <toml++/toml.hpp>
#include <unordered_map>

class ItemRepository {
public:
  explicit ItemRepository(const toml::table &config);

  Item createItem(const std::string &typeName);
  bool exists(const std::string &typeName) const;

  // búsqueda O(1) por catalogId para reconstruir desde disco
  std::optional<Item> findByCatalogId(uint32_t catalogId);

private:
  std::unordered_map<std::string, Item> templates;
  std::unordered_map<uint32_t, std::string>
      catalogIndex; // catalogId → typeName
  uint32_t nextInstanceId = 1;

  ItemSlot parseSlot(const std::string &raw, const std::string &typeName);
  ItemEffect parseEffect(const std::string &raw, const std::string &typeName);
  ItemStats parseStats(const toml::table &entry);
};