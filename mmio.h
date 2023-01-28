#pragma once

#include "interrupt.h"
#include "io_serial.h"
#include "memory.h"

#include <functional>

namespace mpce
{

using namespace std;

class MMIO
{
    /// memory_t segments for KERN mode.
    word_addressible_memory_t kern_code_{"kern_code", 0x1'0000};
    byte_addressible_memory_t kern_data_{"kern_data", 0x1'0000};

    /// memory_t segments for USER mode.
    word_addressible_memory_t user_code_{"user_code", 0x80'0000};
    byte_addressible_memory_t user_data_{"user_data", 0x80'0000};

    /// The mapped io region is pow(2, 12) in size.
    const uint32_t mapped_io_size_ = 0x1000;

    /// The last address directly before the beginning of the mapped io region,
    /// the address preceding 0xf000 is 0xefff.
    const uint32_t mapped_io_begin_ = 0x1'0000 - mapped_io_size_ - 1;

    vector<function<uint16_t()>> mapped_io_load_;

    vector<function<void(uint16_t)>> mapped_io_store_;

    io_serial_interface_t serial_interface_;

    vector<function<void(interrupt_t &)>> irq_notifiers_;

  public:
    ///
    MMIO();

    /// @param is_user_mode
    memory_t &get_code(bool is_user_mode);

    /// @param is_user_mode
    memory_t &get_data(bool is_user_mode);

    /// @returns
    io_serial_interface_t &serial_interface();

    /// @param interrupt
    void irq_notify(interrupt_t &interrupt);

  private:
    /// @param offset
    /// @returns
    uint16_t io_load(const uint32_t offset);

    /// @param offset
    /// @param value
    void io_store(const uint32_t offset, const uint16_t value);
};
} // namespace mpce