#pragma once

#include "item.h"
#include <cstdint>
#include <optional>
#include <string>
#include <toml++/toml.hpp>
#include <unordered_map>

class ItemRepository
{
public:
  explicit ItemRepository(const toml::table &config);

  Item createItem(const std::string &typeName);
  bool exists(const std::string &typeName) const;
  std::optional<Item> findByCatalogId(uint32_t catalogId);

private:
  std::unordered_map<std::string, Item> templates;
  std::unordered_map<uint32_t, std::string> catalogIndex;
  uint32_t nextInstanceId = 1;
};