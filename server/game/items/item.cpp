#include "item.h"

bool Item::isValid() const
{
    return slot != ItemSlot::NONE;
}