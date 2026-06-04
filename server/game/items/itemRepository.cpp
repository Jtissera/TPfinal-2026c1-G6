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

    const auto* catalogIdRaw = entry->get_as<int64_t>("catalog_id");

    if (catalogIdRaw == nullptr) {
      throw std::runtime_error(
          "ItemRepository: item '" + typeName + "' no tiene 'catalog_id'"
      );
    }

    Item tmpl;

    // Nombre lógico interno del server.
    tmpl.typeName = typeName;

    // ID compartido con el JSON del cliente.
    tmpl.catalogId = static_cast<uint32_t>(catalogIdRaw->get());

    // Datos de dominio del server.
    tmpl.slot = parseSlot(slotRaw->get(), typeName);
    tmpl.stats = parseStats(*entry);

    // Efecto opcional.
    if (const auto* effectRaw = entry->get_as<std::string>("effect")) {
      tmpl.effect = parseEffect(effectRaw->get(), typeName);
    } else if (tmpl.slot == ItemSlot::STAFF) {
      // Si es báculo/vara y no especifica effect, asumimos daño mágico.
      tmpl.effect = ItemEffect::DAMAGE;
    } else {
      tmpl.effect = ItemEffect::NONE;
    }

    // Guardamos el template sin instanceId.
    templates[typeName] = std::move(tmpl);
  }
}

Item ItemRepository::createItem(const std::string& typeName) {
  auto it = templates.find(typeName);

  if (it == templates.end()) {
    throw std::out_of_range(
        "ItemRepository: tipo desconocido '" + typeName + "'"
    );
  }

  // Copiamos el template cargado desde TOML.
  Item item = it->second;

  // Esta copia concreta recibe su ID único.
  item.instanceId = nextId++;

  return item;
}

bool ItemRepository::exists(const std::string& typeName) const {
  return templates.find(typeName) != templates.end();
}

ItemSlot ItemRepository::parseSlot(const std::string &raw,
                                   const std::string &typeName) {
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

ItemEffect ItemRepository::parseEffect(const std::string& raw,
                                       const std::string& typeName) {
  if (raw == "heal") {
    return ItemEffect::HEAL;
  }

  if (raw == "mana") {
    return ItemEffect::MANA;
  }

  if (raw == "damage") {
    return ItemEffect::DAMAGE;
  }

  if (raw == "none") {
    return ItemEffect::NONE;
  }

  throw std::runtime_error(
      "ItemRepository: effect desconocido '" + raw +
      "' en item '" + typeName + "'"
  );
}



ItemStats ItemRepository::parseStats(const toml::table& entry) {
  ItemStats stats;

  if (const auto* value = entry.get_as<bool>("is_ranged")) {
    stats.isRanged = value->get();
  }

  if (const auto* value = entry.get_as<int64_t>("damage_min")) {
    stats.damageMin = static_cast<int>(value->get());
  }

  if (const auto* value = entry.get_as<int64_t>("damage_max")) {
    stats.damageMax = static_cast<int>(value->get());
  }

  if (const auto* value = entry.get_as<int64_t>("defense_min")) {
    stats.defenseMin = static_cast<int>(value->get());
  }

  if (const auto* value = entry.get_as<int64_t>("defense_max")) {
    stats.defenseMax = static_cast<int>(value->get());
  }

  if (const auto* value = entry.get_as<int64_t>("mana_cost")) {
    stats.manaCost = static_cast<int>(value->get());
  }

  if (const auto* value = entry.get_as<int64_t>("heal_amount")) {
    stats.healAmount = static_cast<int>(value->get());
  }

  if (const auto* value = entry.get_as<int64_t>("mana_amount")) {
    stats.manaAmount = static_cast<int>(value->get());
  }

  return stats;
}
