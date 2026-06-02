#pragma once

#include <cstdint>
#include <string>

struct ItemDto {
    uint32_t    id       = 0;
    std::string typeName;
    uint8_t     slot     = 0;  
};