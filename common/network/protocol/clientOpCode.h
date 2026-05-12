#pragma once

#include <cstdint>

enum class ClientOpCode : uint8_t
{

    MSG_CONNECT = 0x01,
    MSG_LOGIN = 0x02,
    MSG_CREATE_CHAR = 0x03,
    MSG_DISCONNECT = 0x04,

    MSG_MOVE = 0x10,
    MSG_ATTACK = 0x11,
    MSG_PICK_ITEM = 0x12,
    MSG_DROP_ITEM = 0x13,
    MSG_EQUIP_ITEM = 0x14,
    MSG_MEDITATE = 0x15,
    MSG_RESURRECT = 0x16,

    MSG_TALK_NPC = 0x20,
    MSG_BUY = 0x21,
    MSG_SELL = 0x22,
    MSG_DEPOSIT = 0x23,
    MSG_WITHDRAW = 0x24,
    MSG_LIST_NPC = 0x25,

    MSG_CHAT_PRIVATE = 0x30,
    MSG_CLAN_FOUND = 0x31,
    MSG_CLAN_JOIN = 0x32,
    MSG_CLAN_ACCEPT = 0x33,
    MSG_CLAN_REJECT = 0x34,
    MSG_CLAN_BAN = 0x35,
    MSG_CLAN_KICK = 0x36,
    MSG_CLAN_LEAVE = 0x37,
    MSG_CLAN_LIST = 0x38

};