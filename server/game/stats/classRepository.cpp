#include "classRepository.h"
#include <stdexcept>

ClassRepository::ClassRepository(const toml::table &config) {
  const auto *classesSection = config.get_as<toml::table>("classes");
  if (!classesSection)
    throw std::runtime_error(
        "ClassRepository: falta la seccion [classes] en el TOML");

  for (const auto &[key, value] : *classesSection) {
    const auto *entry = value.as_table();
    if (!entry)
      continue;
    std::string name(key.str());
    classes[name] = parse(name, *entry);
  }
}

const ClassStats &ClassRepository::get(const std::string &className) const {
  auto it = classes.find(className);
  if (it == classes.end())
    throw std::out_of_range("ClassRepository: clase desconocida '" + className +
                            "'");
  return it->second;
}

bool ClassRepository::exists(const std::string &className) const {
  return classes.count(className) > 0;
}

ClassStats ClassRepository::parse(const std::string &name,
                                  const toml::table &entry) const {
  ClassStats stats;
  stats.name = name;
  stats.health = entry["health"].value_or<float>(1.0f);
  stats.mana = entry["mana"].value_or<float>(1.0f);
  stats.meditation = entry["meditation"].value_or<float>(0.0f);
  stats.canUseMagic = entry["can_use_magic"].value_or(false);
  return stats;
}