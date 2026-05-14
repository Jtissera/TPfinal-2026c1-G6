#include "mapSerializer.h"

#include <fstream>
#include <stdexcept>
#include <cstring>
#include <arpa/inet.h>

constexpr uint8_t MapSerializer::MAGIC[8];

// helpers little-endian
static void writeU16(std::ostream& os, uint16_t v) {
    uint16_t le = htole16(v);
    os.write(reinterpret_cast<const char*>(&le), 2);
}

static uint16_t readU16(std::istream& is) {
    uint16_t le = 0;
    is.read(reinterpret_cast<char*>(&le), 2);
    return le16toh(le);
}

void MapSerializer::save(const MapData& map, const std::string& filepath) {
    std::ofstream f(filepath, std::ios::binary | std::ios::trunc);
    if (!f)
        throw std::runtime_error("No se pudo abrir para escribir: " + filepath);

    // Magic
    f.write(reinterpret_cast<const char*>(MAGIC), 8);

    // Version
    writeU16(f, VERSION);

    // Dimensiones
    writeU16(f, map.width());
    writeU16(f, map.height());

    // Nombre
    const std::string& name = map.name();
    writeU16(f, static_cast<uint16_t>(name.size()));
    f.write(name.data(), static_cast<std::streamsize>(name.size()));

    // Tiles
    for (uint16_t y = 0; y < map.height(); ++y) {
        for (uint16_t x = 0; x < map.width(); ++x) {
            const Tile& t = map.at(x, y);
            uint8_t walkable = t.walkable ? 1 : 0;
            f.put(static_cast<char>(t.type));
            f.put(static_cast<char>(t.zone));
            f.put(static_cast<char>(walkable));
            writeU16(f, t.npcId);
        }
    }

    if (!f)
        throw std::runtime_error("Error al escribir el archivo: " + filepath);
}

MapData MapSerializer::load(const std::string& filepath) {
    std::ifstream f(filepath, std::ios::binary);
    if (!f)
        throw std::runtime_error("No se pudo abrir: " + filepath);

    // Verificar magic
    uint8_t magic[8];
    f.read(reinterpret_cast<char*>(magic), 8);
    if (std::memcmp(magic, MAGIC, 8) != 0)
        throw std::runtime_error("Archivo no es un mapa válido de Argentum");

    uint16_t version = readU16(f);
    if (version != VERSION)
        throw std::runtime_error("Versión de mapa no soportada: " +
                                 std::to_string(version));

    uint16_t width  = readU16(f);
    uint16_t height = readU16(f);

    uint16_t nameLen = readU16(f);
    std::string name(nameLen, '\0');
    f.read(name.data(), nameLen);

    MapData map(width, height);
    map.setName(name);

    for (uint16_t y = 0; y < height; ++y) {
        for (uint16_t x = 0; x < width; ++x) {
            Tile t;
            t.type     = static_cast<TileType>(f.get());
            t.zone     = static_cast<ZoneType>(f.get());
            t.walkable = (f.get() != 0);
            t.npcId    = readU16(f);
            map.at(x, y) = t;
        }
    }

    if (!f)
        throw std::runtime_error("Error al leer el archivo: " + filepath);

    return map;
}
