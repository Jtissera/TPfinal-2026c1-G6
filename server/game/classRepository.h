#pragma once
#include <map>
#include <string>
#include "../../../common/game/classStats.h"
#include <toml++/toml.h>

class ClassRepository {
public:
    explicit ClassRepository(const toml::table& config);
    const ClassStats& get(const std::string& className) const;
    bool exists(const std::string& className) const;

private:
    std::map<std::string, ClassStats> classes;
    ClassStats parse(const std::string& name, const toml::table& entry) const;
};