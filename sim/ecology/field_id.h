#pragma once

#include "core/types/types.h"
#include <cstdint>

// Identifies a readable/writable field on a cell
enum class FieldId : u16
{
    None = 0,

    // Environment fields
    Temperature,
    Humidity,
    Fire,
    WindSpeed,

    // Information fields
    Smell,
    Danger,
    Smoke,

    Count
};
