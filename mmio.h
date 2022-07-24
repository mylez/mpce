#pragma once

#include "memory.h"

namespace MPCE
{
class MMIO
{
    /// @brief Memory segments for KERN mode.
    WordAddressibleMemory kern_code_{"kern_code", 0x1'0000};
    ByteAddressibleMemory kern_data_{"kern_data", 0x1'0000};

    /// @brief Memory segments for USER mode.
    WordAddressibleMemory user_code_{"user_code", 0x80'0000};
    ByteAddressibleMemory user_data_{"user_data", 0x80'0000};

  public:
    Memory &get_code(bool is_user_mode)
    {
        return is_user_mode ? user_code_ : kern_code_;
    }

    Memory &get_data(bool is_user_mode)
    {
        return is_user_mode ? user_data_ : kern_data_;
    }
};
} // namespace MPCE