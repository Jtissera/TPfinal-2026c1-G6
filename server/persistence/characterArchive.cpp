#include "characterArchive.h"

#include <cstring>
#include <iostream>

namespace
{
    const std::size_t CHAR_IDX_NAME_LEN = 32;
}

CharacterArchive::CharacterArchive(const std::string &datPath,
                                   const std::string &indexPath)
    : archive_(datPath, indexPath, sizeof(CharacterRecord), CHAR_IDX_NAME_LEN)
{
}

bool CharacterArchive::exists(const std::string &name) const
{
    return archive_.hasKey(name);
}

bool CharacterArchive::save(const std::string &name,
                            const std::string &race,
                            const std::string &cls)
{
    if (archive_.hasKey(name))
    {
        std::cerr << "[CharacterArchive] '" << name << "' ya existe." << std::endl;
        return false;
    }

    CharacterRecord rec;
    std::memset(&rec, 0, sizeof(rec));
    std::strncpy(rec.name, name.c_str(), sizeof(rec.name) - 1);
    std::strncpy(rec.race, race.c_str(), sizeof(rec.race) - 1);
    std::strncpy(rec.cls, cls.c_str(), sizeof(rec.cls) - 1);

    const uint64_t offset = archive_.allocateSlot(name);
    archive_.writeRecord(offset, &rec, sizeof(rec));
    archive_.appendToIndex(name, offset);

    std::cout << "[CharacterArchive] Guardado: '" << name
              << "' raza='" << race << "' clase='" << cls << "'" << std::endl;
    return true;
}

std::optional<CharacterRecord> CharacterArchive::load(const std::string &name) const
{
    std::optional<uint64_t> offset = archive_.getOffset(name);
    if (!offset)
        return std::nullopt;

    CharacterRecord rec;
    if (!archive_.readRecord(*offset, &rec, sizeof(rec)))
        return std::nullopt;

    return rec;
}

bool CharacterArchive::updateClan(const std::string &name,
                                  const std::string &clanName)
{
    std::optional<uint64_t> offset = archive_.getOffset(name);
    if (!offset)
    {
        std::cerr << "[CharacterArchive] updateClan: '" << name
                  << "' no existe." << std::endl;
        return false;
    }

    CharacterRecord rec;
    if (!archive_.readRecord(*offset, &rec, sizeof(rec)))
        return false;

    std::memset(rec.clanName, 0, sizeof(rec.clanName));
    std::strncpy(rec.clanName, clanName.c_str(), sizeof(rec.clanName) - 1);

    archive_.writeRecord(*offset, &rec, sizeof(rec));

    std::cout << "[CharacterArchive] '" << name << "' clan actualizado a '"
              << (clanName.empty() ? "(ninguno)" : clanName) << "'" << std::endl;
    return true;
}