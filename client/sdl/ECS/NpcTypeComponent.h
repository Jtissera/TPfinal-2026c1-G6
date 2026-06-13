#pragma once

#include "ECS.h"
#include "common/npcType.h"

struct NpcTypeComponent : public Component
{
    NpcType type;
    explicit NpcTypeComponent(NpcType t) : type(t) {}
};