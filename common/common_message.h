#ifndef COMMON_MESSAGE_H_
#define COMMON_MESSAGE_H_

#include <string>
#include <cstdint>

// PLACEHOLDER: espero protocolo
struct Message {
    uint8_t type = 0;
    std::string text;
};

#endif