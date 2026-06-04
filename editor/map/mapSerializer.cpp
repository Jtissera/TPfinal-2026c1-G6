#include "mapSerializer.h"
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <arpa/inet.h>

constexpr uint8_t MapSerializer::MAGIC[8];

static void writeU16(std::ostream &os, uint16_t v)
{
    uint16_t le = htole16(v);
    os.write(reinterpret_cast<const char *>(&le), 2);
}

static uint16_t readU16(std::istream &is)
{
    uint16_t le = 0;
    is.read(reinterpret_cast<char *>(&le), 2);
    return le16toh(le);
}

void MapSerializer::save(const MapData &map, const std::string &filepath)
{
    std::ofstream f(filepath, std::ios::binary | std::ios::trunc);
    if (!f)
        throw std::runtime_error("No se pudo abrir para escribir: " + filepath);

    f.write(reinterpret_cast<const char *>(MAGIC), 8);
    writeU16(f, VERSION);
    f.put(static_cast<char>(map.mapType())); // v3: tipo de mapa
    writeU16(f, map.width());
    writeU16(f, map.height());

    const std::string &name = map.name();
    writeU16(f, static_cast<uint16_t>(name.size()));
    f.write(name.data(), static_cast<std::streamsize>(name.size()));

    for (uint16_t y = 0; y < map.height(); ++y)
    {
        for (uint16_t x = 0; x < map.width(); ++x)
        {
            const Tile &t = map.at(x, y);
            f.put(static_cast<char>(t.type));
            f.put(static_cast<char>(t.zone));
            f.put(static_cast<char>(t.walkable ? 1 : 0));
            f.put(static_cast<char>(t.npc));
            writeU16(f, static_cast<uint16_t>(t.targetMap.size()));
            f.write(t.targetMap.data(),
                    static_cast<std::streamsize>(t.targetMap.size()));
        }
    }

    if (!f)
        throw std::runtime_error("Error al escribir: " + filepath);
}

MapData MapSerializer::load(const std::string &filepath)
{
    std::ifstream f(filepath, std::ios::binary);
    if (!f)
        throw std::runtime_error("No se pudo abrir: " + filepath);

    uint8_t magic[8];
    f.read(reinterpret_cast<char *>(magic), 8);
    if (std::memcmp(magic, MAGIC, 8) != 0)
        throw std::runtime_error("Archivo no es un mapa válido de Argentum");

    uint16_t version = readU16(f);
    if (version < 1 || version > VERSION)
        throw std::runtime_error("Versión de mapa no soportada: " +
                                 std::to_string(version));

    // v3: leer MapType; versiones anteriores asumen WORLD
    MapType mapType = MapType::WORLD;
    if (version >= 3)
        mapType = static_cast<MapType>(f.get());

    uint16_t width = readU16(f);
    uint16_t height = readU16(f);

    uint16_t nameLen = readU16(f);
    std::string name(nameLen, '\0');
    f.read(name.data(), nameLen);

    MapData map(width, height, mapType);
    map.setName(name);

    for (uint16_t y = 0; y < height; ++y)
    {
        for (uint16_t x = 0; x < width; ++x)
        {
            Tile t;
            t.type = static_cast<TileType>(f.get());
            t.zone = static_cast<ZoneType>(f.get());
            t.walkable = (f.get() != 0);
            t.npc = static_cast<NpcType>(f.get());

            // targetMap presente desde v2
            if (version >= 2)
            {
                uint16_t tlen = readU16(f);
                t.targetMap.resize(tlen);
                f.read(t.targetMap.data(), tlen);
            }
            map.at(x, y) = t;
        }
    }

    if (!f)
        throw std::runtime_error("Error al leer: " + filepath);

    return map;
}