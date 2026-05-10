#include "common/version.h"

#include <string>

namespace argentum {

std::string version_string() {
    return std::to_string(VERSION_MAJOR) + "." +
           std::to_string(VERSION_MINOR) + "." +
           std::to_string(VERSION_PATCH);
}

}
