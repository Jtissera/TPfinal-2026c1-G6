#pragma once
#include "item.h"
#include <optional>
#include <string>
#include <unordered_map>

class ItemRepository {
public:
  explicit ItemRepository(const toml::table &config);

  Item createItem(const std::string &typeName);
  bool exists(const std::string &typeName) const;

private:
  struct ItemTemplate {
    ItemSlot slot;
    ItemStats stats;
  };

  std::unordered_map<std::string, ItemTemplate> templates;
  uint32_t nextId = 1;

  ItemSlot parseSlot(const std::string &raw, const std::string &typeName);
  ItemEffect parseEffect(const std::string &raw, const std::string &typeName);
  ItemStats parseStats(const toml::table &entry);
};