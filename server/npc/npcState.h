#pragma once
enum class NpcState { IDLE, CHASING, ATTACKING, RETURNING};

enum class NpcKind {
    Passive,
    Hostile
};

enum class NpcRole {
    Merchant,
    Priest,
    Banker,
    Creature
};
enum class NpcLifeState { ALIVE, RESPAWNING };