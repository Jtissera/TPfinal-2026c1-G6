#pragma once
#include "bankAccount.h"
#include <unordered_map>
#include <cstdint>

class BankRepository
{
public:
    BankAccount &get(uint32_t playerId);
    const BankAccount &get(uint32_t playerId) const;

private:
    std::unordered_map<uint32_t, BankAccount> accounts;
};