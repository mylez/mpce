#pragma once

#include "memory.h"

namespace MPCE
{
class MMIO
{
  public:
    /// @brief Memory segments for KERN mode.
    WordAddressibleMemory kern_code{"kern_code", 0x1'0000};
    ByteAddressibleMemory kern_data{"kern_data", 0x1'0000};

    /// @brief Memory segments for USER mode.
    WordAddressibleMemory user_code{"user_code", 0x80'0000};
    ByteAddressibleMemory user_data{"user_data", 0x80'0000};

    WordAddressibleMemory &get_code(bool is_user_mode)
    {
        return is_user_mode ? user_code : kern_code;
    }
};
} // namespace MPCE