#include "bankRepository.h"

BankAccount &BankRepository::get(uint32_t playerId)
{
    return accounts[playerId];
}

const BankAccount &BankRepository::get(uint32_t playerId) const
{
    static const BankAccount empty;
    auto it = accounts.find(playerId);
    return it != accounts.end() ? it->second : empty;
}