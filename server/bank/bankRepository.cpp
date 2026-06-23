#include "bankRepository.h"

BankAccount &BankRepository::get(uint32_t playerId)
{
    return accounts[playerId];
}

const BankAccount &BankRepository::get(uint32_t playerId) const
{
    std::unordered_map<uint32_t, BankAccount>::const_iterator it = accounts.find(playerId);
    return it != accounts.end() ? it->second : emptyAccount;
}