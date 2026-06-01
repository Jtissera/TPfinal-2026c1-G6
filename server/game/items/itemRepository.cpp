#include "itemRepository.h"
#include <stdexcept>
#include <string>

ItemRepository::ItemRepository(const toml::table &config) {

  const auto *itemsSection = config.get_as<toml::table>("items");
  if (!itemsSection)
    throw std::runtime_error(
        "ItemRepository: falta la seccion [items] en el TOML");

  for (const auto &[key, value] : *itemsSection) {
    const auto *entry = value.as_table();
    if (!entry)
      continue; // ignora valores que no sean tablas

    const std::string typeName(key.str());

    auto slotRaw = entry->get_as<std::string>("slot");

    if (!slotRaw)
      throw std::runtime_error("ItemRepository: item '" + typeName +
                               "' no tiene 'slot'");

    Item tmpl;
    tmpl.typeName = typeName;
    tmpl.slot = parseSlot(slotRaw->get(), typeName);
    tmpl.stats = parseStats(*entry);

    if (const auto *effectRaw = entry->get_as<std::string>("effect")) {
      tmpl.effect = parseEffect(effectRaw->get(), typeName);
    } else if (tmpl.slot == ItemSlot::STAFF) {
      tmpl.effect = ItemEffect::DAMAGE; // staffs dañinos por defecto
    }

    templates[typeName] = std::move(tmpl);
  }
}

Item ItemRepository::createItem(const std::string &typeName) {
  auto it = templates.find(typeName);
  if (it == templates.end())
    throw std::out_of_range("ItemRepository: tipo desconocido '" + typeName +
                            "'");

  return Item{nextId++, it->second.typeName, it->second.slot, it->second.effect,
              it->second.stats};
}

bool ItemRepository::exists(const std::string &typeName) const {
  return templates.count(typeName) > 0;
}

ItemSlot ItemRepository::parseSlot(const std::string &raw,
                                   const std::string &typeName) {
  const std::map<std::string, ItemSlot> mapping = {
      {"WEAPON", ItemSlot::WEAPON}, {"STAFF", ItemSlot::STAFF},
      {"ARMOR", ItemSlot::ARMOR},   {"HELMET", ItemSlot::HELMET},
      {"SHIELD", ItemSlot::SHIELD}, {"CONSUMABLE", ItemSlot::CONSUMABLE},
  };

  auto it = mapping.find(raw);
  if (it == mapping.end())
    throw std::runtime_error("ItemRepository: slot desconocido '" + raw +
                             "' en item '" + typeName + "'");
  return it->second;
}

ItemEffect ItemRepository::parseEffect(const std::string &raw,
                                       const std::string &typeName) {
  const std::map<std::string, ItemEffect> mapping = {
      {"damage", ItemEffect::DAMAGE},
      {"heal", ItemEffect::HEAL},
  };

  auto it = mapping.find(raw);
  if (it == mapping.end())
    throw std::runtime_error("ItemRepository: effect desconocido '" + raw +
                             "' en item '" + typeName + "'");
  return it->second;
}

ItemStats ItemRepository::parseStats(const toml::table &entry) {
  ItemStats stats;
  stats.damageMin = entry["damage_min"].value_or<uint16_t>(0);
  stats.damageMax = entry["damage_max"].value_or<uint16_t>(0);
  stats.defenseMin = entry["defense_min"].value_or<uint16_t>(0);
  stats.defenseMax = entry["defense_max"].value_or<uint16_t>(0);
  stats.healAmount = entry["heal_amount"].value_or<uint16_t>(0);
  stats.manaAmount = entry["mana_amount"].value_or<uint16_t>(0);
  stats.manaCost = entry["mana_cost"].value_or<uint16_t>(0);
  stats.isRanged = entry["is_ranged"].value_or(false);

  return stats;
}
