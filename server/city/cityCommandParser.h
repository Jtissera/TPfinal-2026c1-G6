#pragma once
#include <string>
#include <optional>
#include <cstdint>

struct CityCommand
{
    enum class Type
    {
        RESURRECT,
        HEAL,
        BUY,
        SELL,
        DEPOSIT,
        WITHDRAW,
    };

    Type type;
    std::string itemName;    // vacío si no aplica
    uint32_t goldAmount = 0; // solo para depositar/retirar oro
    bool isGold = false;
};

class CityCommandParser
{
public:
    // Retorna nullopt si el comando es inválido o desconocido.
    static std::optional<CityCommand> parse(const std::string &raw);
};