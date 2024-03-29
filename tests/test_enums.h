#pragma once

#include "utils/enum.h"
#include "utils/formatter/enum.h"

namespace test_enums {

// enum for test meta_enum
META_ENUM_(Type, int, TypeA, TypeB, TypeC);

// enum for test meta_enum with bitmask
META_ENUM_(Flag, int, FlagA = 1 << 0, FlagB = 1 << 1, FlagC = 1 << 2,
                FlagD = FlagA | FlagB | FlagC, FlagE = FlagB | FlagC);
BITMASK_DEFINE_MAX_ELEMENT(Flag, FlagC)

// enum for test bitmask
enum class Bit : int { BitA = 1 << 0, BitB = 1 << 1, BitC = 1 << 2 };
BITMASK_DEFINE_MAX_ELEMENT(Bit, BitC)

}  // namespace formatter_test_enums
