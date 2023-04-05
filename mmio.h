#pragma once

#include "interrupt.h"
#include "io_serial.h"
#include "ram.h"

#include <functional>

using namespace std;

class MMIO
{
    /// ram segments for KERN mode.
    ram_t kern_code_{0x1'0000};
    ram_t kern_data_{0x1'0000};

    /// ram segments for USER mode.
    ram_t user_code_{0x80'0000};
    ram_t user_data_{0x80'0000};

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
    ram_t &get_code(bool is_user_mode);

    /// @param is_user_mode
    ram_t &get_data(bool is_user_mode);

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