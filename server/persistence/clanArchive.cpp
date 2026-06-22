#include "clanArchive.h"
#include <cstring>
#include <filesystem>
#include <iostream>

// Entrada del índice: [char name[32]][uint64_t offset] = 40 bytes
static constexpr std::size_t CLAN_IDX_NAME_LEN = 32;
static constexpr std::size_t CLAN_IDX_ENTRY_SIZE =
    CLAN_IDX_NAME_LEN + sizeof(uint64_t);

ClanArchive::ClanArchive(const std::string &datPath,
                         const std::string &indexPath)
    : datPath_(datPath), indexPath_(indexPath)
{
  std::filesystem::path p(datPath_);
  if (p.has_parent_path())
    std::filesystem::create_directories(p.parent_path());

  if (!std::filesystem::exists(datPath_))
  {
    std::ofstream f(datPath_, std::ios::binary);
  }
  if (!std::filesystem::exists(indexPath_))
  {
    std::ofstream f(indexPath_, std::ios::binary);
  }

  loadIndex();
}

void ClanArchive::loadIndex()
{
  std::ifstream f(indexPath_, std::ios::binary);
  if (!f.is_open())
    return;

  char nameBuf[CLAN_IDX_NAME_LEN];
  uint64_t offset = 0;

  while (f.read(nameBuf, CLAN_IDX_NAME_LEN) &&
         f.read(reinterpret_cast<char *>(&offset), sizeof(uint64_t)))
  {
    std::string name(nameBuf, strnlen(nameBuf, CLAN_IDX_NAME_LEN));
    index_[name] = offset;
  }
}

bool ClanArchive::exists(const std::string &clanName) const
{
  std::shared_lock lock(mutex_);
  return index_.count(clanName) > 0;
}

ClanRecord ClanArchive::toRecord(const Clan &clan) const
{
  ClanRecord rec;
  std::memset(&rec, 0, sizeof(rec));

  std::strncpy(rec.name, clan.getName().c_str(), sizeof(rec.name) - 1);
  std::strncpy(rec.founderNick, clan.getFounderNick().c_str(),
               sizeof(rec.founderNick) - 1);

  const auto &members = clan.getMembers();
  rec.memberCount = static_cast<uint8_t>(
      std::min(members.size(), CLAN_REC_MAX_MEMBERS));
  {
    std::size_t i = 0;
    for (const auto &nick : members)
    {
      if (i >= CLAN_REC_MAX_MEMBERS)
        break;
      std::strncpy(rec.members[i], nick.c_str(), CLAN_REC_NAME_LEN - 1);
      ++i;
    }
  }

  const auto &applicants = clan.getApplicants();
  rec.applicantCount = static_cast<uint8_t>(
      std::min(applicants.size(), CLAN_REC_MAX_APPLICANTS));
  {
    std::size_t i = 0;
    for (const auto &nick : applicants)
    {
      if (i >= CLAN_REC_MAX_APPLICANTS)
        break;
      std::strncpy(rec.applicants[i], nick.c_str(), CLAN_REC_NAME_LEN - 1);
      ++i;
    }
  }

  const auto &banned = clan.getBannedPlayers();
  rec.bannedCount = static_cast<uint8_t>(
      std::min(banned.size(), CLAN_REC_MAX_BANNED));
  {
    std::size_t i = 0;
    for (const auto &nick : banned)
    {
      if (i >= CLAN_REC_MAX_BANNED)
        break;
      std::strncpy(rec.bannedPlayers[i], nick.c_str(), CLAN_REC_NAME_LEN - 1);
      ++i;
    }
  }

  return rec;
}

Clan ClanArchive::fromRecord(const ClanRecord &rec) const
{
  std::string name(rec.name, strnlen(rec.name, sizeof(rec.name)));
  std::string founder(rec.founderNick,
                      strnlen(rec.founderNick, sizeof(rec.founderNick)));

  Clan clan(name, founder, CLAN_REC_MAX_MEMBERS);

  // El fundador entra como miembro vía el constructor de Clan en
  // ClanManager::foundClan normalmente, pero acá reconstruimos desde
  // cero: agregamos todos los miembros guardados explícitamente
  // (incluye al fundador si estaba en la lista, que siempre debería).
  for (uint8_t i = 0; i < rec.memberCount; ++i)
  {
    std::string nick(rec.members[i],
                     strnlen(rec.members[i], CLAN_REC_NAME_LEN));
    clan.addMember(nick);
  }

  for (uint8_t i = 0; i < rec.applicantCount; ++i)
  {
    std::string nick(rec.applicants[i],
                     strnlen(rec.applicants[i], CLAN_REC_NAME_LEN));
    clan.addApplicant(nick);
  }

  // Los banneados no tienen un "addBanned" directo en Clan; usamos
  // banPlayer, que también limpia member/applicant pero como nunca
  // fueron agregados acá no tiene efecto colateral.
  for (uint8_t i = 0; i < rec.bannedCount; ++i)
  {
    std::string nick(rec.bannedPlayers[i],
                     strnlen(rec.bannedPlayers[i], CLAN_REC_NAME_LEN));
    clan.banPlayer(nick);
  }

  return clan;
}

void ClanArchive::save(const Clan &clan)
{
  std::unique_lock lock(mutex_);

  const std::string &name = clan.getName();
  ClanRecord rec = toRecord(clan);

  uint64_t offset;
  bool isNew = false;

  auto it = index_.find(name);
  if (it != index_.end())
  {
    offset = it->second;
  }
  else
  {
    offset = static_cast<uint64_t>(index_.size()) * sizeof(ClanRecord);
    index_[name] = offset;
    isNew = true;
  }

  std::fstream dat(datPath_, std::ios::binary | std::ios::in | std::ios::out);
  dat.seekp(static_cast<std::streamoff>(offset));
  dat.write(reinterpret_cast<const char *>(&rec), sizeof(rec));
  dat.flush();

  if (isNew)
  {
    std::ofstream idx(indexPath_, std::ios::binary | std::ios::app);
    char nameBuf[CLAN_IDX_NAME_LEN] = {};
    std::strncpy(nameBuf, name.c_str(), CLAN_IDX_NAME_LEN - 1);
    idx.write(nameBuf, CLAN_IDX_NAME_LEN);
    idx.write(reinterpret_cast<const char *>(&offset), sizeof(uint64_t));
    idx.flush();
  }

  std::cout << "[ClanArchive] Guardado clan '" << name << "' members="
            << static_cast<int>(rec.memberCount) << std::endl;
}

std::optional<Clan> ClanArchive::load(const std::string &clanName) const
{
  uint64_t offset = 0;
  {
    std::shared_lock lock(mutex_);
    auto it = index_.find(clanName);
    if (it == index_.end())
      return std::nullopt;
    offset = it->second;
  }

  std::ifstream dat(datPath_, std::ios::binary);
  if (!dat.is_open())
    return std::nullopt;

  dat.seekg(static_cast<std::streamoff>(offset));
  ClanRecord rec;
  dat.read(reinterpret_cast<char *>(&rec), sizeof(rec));
  if (!dat)
    return std::nullopt;

  return fromRecord(rec);
}

std::vector<Clan> ClanArchive::loadAll() const
{
  std::shared_lock lock(mutex_);

  std::vector<Clan> result;
  if (index_.empty())
    return result;

  std::ifstream dat(datPath_, std::ios::binary);
  if (!dat.is_open())
    return result;

  for (const auto &[name, offset] : index_)
  {
    dat.seekg(static_cast<std::streamoff>(offset));
    ClanRecord rec;
    dat.read(reinterpret_cast<char *>(&rec), sizeof(rec));
    if (dat)
      result.push_back(fromRecord(rec));
  }

  return result;
}