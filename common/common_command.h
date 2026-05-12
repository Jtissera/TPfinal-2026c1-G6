#ifndef COMMON_COMMAND_H_
#define COMMON_COMMAND_H_

#include <string>
#include <cstdint>

// PLACEHOLDER: estructura minima hasta que el protocolo definitivo este listo.
// Cuando el protocolo este vemos.
struct Command {
    uint8_t type = 0;
    std::string playerName;
};

#endif