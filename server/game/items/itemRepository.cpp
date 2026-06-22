#include "itemRepository.h"
#include <stdexcept>
#include <string>

namespace
{
  ItemSlot parseSlot(const std::string &raw, const std::string &typeName)
  {
    if (raw == "WEAPON")
      return ItemSlot::WEAPON;
    if (raw == "STAFF")
      return ItemSlot::STAFF;
    if (raw == "ARMOR")
      return ItemSlot::ARMOR;
    if (raw == "HELMET")
      return ItemSlot::HELMET;
    if (raw == "SHIELD")
      return ItemSlot::SHIELD;
    if (raw == "CONSUMABLE")
      return ItemSlot::CONSUMABLE;
    throw std::runtime_error("ItemRepository: slot desconocido '" + raw +
                             "' en item '" + typeName + "'");
  }

  ItemEffect parseEffect(const std::string &raw, const std::string &typeName)
  {
    if (raw == "heal")
      return ItemEffect::HEAL;
    if (raw == "mana")
      return ItemEffect::MANA;
    if (raw == "damage")
      return ItemEffect::DAMAGE;
    if (raw == "none")
      return ItemEffect::NONE;
    throw std::runtime_error("ItemRepository: effect desconocido '" + raw +
                             "' en item '" + typeName + "'");
  }

  ItemStats parseStats(const toml::table &entry)
  {
    ItemStats stats;

    const toml::value<bool> *isRanged = entry.get_as<bool>("is_ranged");
    if (isRanged)
      stats.isRanged = isRanged->get();

    const toml::value<int64_t> *damageMin = entry.get_as<int64_t>("damage_min");
    if (damageMin)
      stats.damageMin = static_cast<uint16_t>(damageMin->get());

    const toml::value<int64_t> *damageMax = entry.get_as<int64_t>("damage_max");
    if (damageMax)
      stats.damageMax = static_cast<uint16_t>(damageMax->get());

    const toml::value<int64_t> *defenseMin = entry.get_as<int64_t>("defense_min");
    if (defenseMin)
      stats.defenseMin = static_cast<uint16_t>(defenseMin->get());

    const toml::value<int64_t> *defenseMax = entry.get_as<int64_t>("defense_max");
    if (defenseMax)
      stats.defenseMax = static_cast<uint16_t>(defenseMax->get());

    const toml::value<int64_t> *manaCost = entry.get_as<int64_t>("mana_cost");
    if (manaCost)
      stats.manaCost = static_cast<uint16_t>(manaCost->get());

    const toml::value<int64_t> *healAmount = entry.get_as<int64_t>("heal_amount");
    if (healAmount)
      stats.healAmount = static_cast<uint16_t>(healAmount->get());

    const toml::value<int64_t> *manaAmount = entry.get_as<int64_t>("mana_amount");
    if (manaAmount)
      stats.manaAmount = static_cast<uint16_t>(manaAmount->get());

    return stats;
  }
}

ItemRepository::ItemRepository(const toml::table &config)
{
  const toml::table *itemsSection = config.get_as<toml::table>("items");
  if (!itemsSection)
    throw std::runtime_error(
        "ItemRepository: falta la seccion [items] en el TOML");

  for (const auto &entry : *itemsSection)
  {
    const toml::table *itemTable = entry.second.as_table();
    if (!itemTable)
      continue;

    const std::string typeName(entry.first.str());

    const toml::value<std::string> *slotRaw = itemTable->get_as<std::string>("slot");
    if (!slotRaw)
      throw std::runtime_error("ItemRepository: item '" + typeName +
                               "' no tiene 'slot'");

    const toml::value<int64_t> *catalogIdRaw = itemTable->get_as<int64_t>("catalog_id");
    if (!catalogIdRaw)
      throw std::runtime_error("ItemRepository: item '" + typeName +
                               "' no tiene 'catalog_id'");

    Item tmpl;
    tmpl.typeName = typeName;
    tmpl.catalogId = static_cast<uint32_t>(catalogIdRaw->get());
    tmpl.slot = parseSlot(slotRaw->get(), typeName);
    tmpl.stats = parseStats(*itemTable);

    const toml::value<std::string> *effectRaw = itemTable->get_as<std::string>("effect");
    if (effectRaw)
    {
      tmpl.effect = parseEffect(effectRaw->get(), typeName);
    }
    else if (tmpl.slot == ItemSlot::STAFF)
    {
      tmpl.effect = ItemEffect::DAMAGE;
    }
    else
    {
      tmpl.effect = ItemEffect::NONE;
    }

    catalogIndex[tmpl.catalogId] = typeName;
    templates[typeName] = std::move(tmpl);
  }
}

Item ItemRepository::createItem(const std::string &typeName)
{
  std::unordered_map<std::string, Item>::iterator it = templates.find(typeName);
  if (it == templates.end())
    throw std::out_of_range("ItemRepository: tipo desconocido '" + typeName + "'");

  Item item = it->second;
  item.instanceId = nextInstanceId++;
  return item;
}

bool ItemRepository::exists(const std::string &typeName) const
{
  return templates.find(typeName) != templates.end();
}

std::optional<Item> ItemRepository::findByCatalogId(uint32_t catalogId)
{
  std::unordered_map<uint32_t, std::string>::iterator it = catalogIndex.find(catalogId);
  if (it == catalogIndex.end())
    return std::nullopt;
  return createItem(it->second);
}