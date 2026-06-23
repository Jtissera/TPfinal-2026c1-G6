#pragma once

#include "bankAccount.h"
#include <cstdint>
#include <unordered_map>

class BankRepository
{
public:
    BankAccount &get(uint32_t playerId);
    const BankAccount &get(uint32_t playerId) const;

private:
    std::unordered_map<uint32_t, BankAccount> accounts;
    BankAccount emptyAccount;
};