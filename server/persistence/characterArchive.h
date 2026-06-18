#pragma once
#include <fstream>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>

struct CharacterRecord {
  char name[32] = {};
  char race[32] = {};
  char cls[32] = {};
};
static_assert(sizeof(CharacterRecord) == 96, "CharacterRecord size mismatch");

class CharacterArchive {
public:
  CharacterArchive(const std::string &datPath, const std::string &indexPath);

  // Devuelve false si el nombre ya existe.
  bool save(const std::string &name, const std::string &race,
            const std::string &cls);

  bool exists(const std::string &name) const;

  std::optional<CharacterRecord> load(const std::string &name) const;

private:
  void loadIndex();

  std::string datPath_;
  std::string indexPath_;

  // índice en memoria: nombre → offset en dat
  std::unordered_map<std::string, uint64_t> index_;
  mutable std::shared_mutex mutex_;
};