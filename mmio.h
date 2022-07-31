#pragma once

#include "interrupt.h"
#include "io_serial.h"
#include "memory.h"

#include <functional>

namespace MPCE
{

class MMIO
{
    /// Memory segments for KERN mode.
    WordAddressibleMemory kern_code_{"kern_code", 0x1'0000};
    ByteAddressibleMemory kern_data_{"kern_data", 0x1'0000};

    /// Memory segments for USER mode.
    WordAddressibleMemory user_code_{"user_code", 0x80'0000};
    ByteAddressibleMemory user_data_{"user_data", 0x80'0000};

    /// The mapped io region is pow(2, 12) in size.
    const uint32_t mapped_io_size_ = 0x1000;

    /// The last address directly before the beginning of the mapped io region,
    /// the address preceding 0xf000 is 0xefff.
    const uint32_t mapped_io_begin_ = 0x1'0000 - mapped_io_size_ - 1;

    std::vector<std::function<uint16_t(uint32_t)>> mapped_io_load_;

    std::vector<std::function<void(uint32_t, uint16_t)>> mapped_io_store_;

    IOSerialInterface serial_interface_;

  public:
    MMIO()
        : mapped_io_load_{mapped_io_size_, [](uint32_t) { return 0; }},
          mapped_io_store_{mapped_io_size_, [](uint32_t, uint32_t) {}}
    {
        using namespace std::placeholders;

        kern_data_.map_io(mapped_io_begin_, std::bind(&MMIO::io_load, this, _1),
                          std::bind(&MMIO::io_store, this, _1, _2));

        /// Register the mapped IO handlers for the serial console.

        mapped_io_load_[0x00] =
            std::bind(&IOSerialInterface::mmio_read, &serial_interface_);

        mapped_io_store_[0x00] =
            std::bind(&IOSerialInterface::mmio_write, &serial_interface_, _1);

        mapped_io_store_[0x01] = std::bind(
            &IOSerialInterface::mmio_buffer_nonempty, &serial_interface_);
    }

    Memory &get_code(bool is_user_mode)
    {
        return is_user_mode ? user_code_ : kern_code_;
    }

    Memory &get_data(bool is_user_mode)
    {
        return is_user_mode ? user_data_ : kern_data_;
    }

    uint16_t io_load(uint32_t phys_addr)
    {
        printf("mapped io_load: phys_addr=%4x\n", phys_addr);
        return 0;
    }

    void io_store(uint32_t phys_addr, uint16_t value)
    {
        printf("mapped io_load: phys_addr=%4x, value=%4x\n", phys_addr, value);
    }

  private:
};
} // namespace MPCE