#pragma once
#include <fstream>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "../game/clan/clan.h"

// Toml
static constexpr std::size_t CLAN_REC_NAME_LEN       = 32;
static constexpr std::size_t CLAN_REC_MAX_MEMBERS    = 16;  
static constexpr std::size_t CLAN_REC_MAX_APPLICANTS = 32;
static constexpr std::size_t CLAN_REC_MAX_BANNED     = 64;

struct ClanRecord {
  uint8_t memberCount    = 0;
  uint8_t applicantCount = 0;
  uint8_t bannedCount    = 0;
  uint8_t _pad           = 0;

  char name[CLAN_REC_NAME_LEN]        = {};
  char founderNick[CLAN_REC_NAME_LEN] = {};

  char members[CLAN_REC_MAX_MEMBERS][CLAN_REC_NAME_LEN]       = {};
  char applicants[CLAN_REC_MAX_APPLICANTS][CLAN_REC_NAME_LEN] = {};
  char bannedPlayers[CLAN_REC_MAX_BANNED][CLAN_REC_NAME_LEN]  = {};
};
static_assert(std::is_trivially_copyable_v<ClanRecord>, "must be POD");

// Persistencia de clanes. Mismo patrón liviano que CharacterArchive:
// archivo de datos + índice nombre-de-clan -> offset en memoria.
// Sin hilo propio: los clanes cambian con poca frecuencia (fundar,
// aceptar/rechazar, ban, kick, leave) así que cada cambio se persiste
// de forma síncrona en el momento en que ocurre.
class ClanArchive {
public:
  ClanArchive(const std::string &datPath, const std::string &indexPath);

  // Se llama tras CUALQUIER cambio de estado del clan.
  void save(const Clan &clan); //es bajo demanda, esto puede no estar funcionando

  bool exists(const std::string &clanName) const;

  std::optional<Clan> load(const std::string &clanName) const;

  std::vector<Clan> loadAll() const;

private:
  void loadIndex();
  ClanRecord toRecord(const Clan &clan) const;
  Clan fromRecord(const ClanRecord &rec) const;

  std::string datPath_;
  std::string indexPath_;

  std::unordered_map<std::string, uint64_t> index_;
  mutable std::shared_mutex mutex_;
};