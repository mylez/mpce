#pragma once

#include "interrupt.h"
#include "io_serial.h"
#include "memory.h"

#include <functional>

namespace MPCE
{

using namespace std;

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

    vector<function<uint16_t()>> mapped_io_load_;

    vector<function<void(uint16_t)>> mapped_io_store_;

    IOSerialInterface serial_interface_;

    vector<function<void(Interrupt &)>> irq_notifiers_;

  public:
    ///
    MMIO() : mapped_io_load_{mapped_io_size_}, mapped_io_store_{mapped_io_size_}
    {
        using namespace placeholders;

        LOG(INFO) << "initializing mmio\n";

        kern_data_.map_io(mapped_io_begin_, bind(&MMIO::io_load, this, _1),
                          bind(&MMIO::io_store, this, _1, _2));

        /// Register the mapped IO handlers for the serial console.

        mapped_io_load_.at(0x00) =
            bind(&IOSerialInterface::mmio_read, &serial_interface_);

        mapped_io_store_.at(0x00) =
            bind(&IOSerialInterface::mmio_write, &serial_interface_, _1);

        mapped_io_store_.at(0x01) =
            bind(&IOSerialInterface::mmio_buffer_nonempty, &serial_interface_);

        irq_notifiers_.push_back(
            bind(&IOSerialInterface::mmio_irq_notify, &serial_interface_, _1));
    }

    /// @param is_user_mode
    Memory &get_code(bool is_user_mode)
    {
        return is_user_mode ? user_code_ : kern_code_;
    }

    /// @param is_user_mode
    Memory &get_data(bool is_user_mode)
    {
        return is_user_mode ? user_data_ : kern_data_;
    }

    /// @returns
    IOSerialInterface &serial_interface()
    {
        return serial_interface_;
    }

    /// @param interrupt
    void irq_notify(Interrupt &interrupt)
    {
        for (auto notify : irq_notifiers_)
        {
            notify(interrupt);
        }
    }

  private:
    /// @param offset
    /// @returns
    uint16_t io_load(const uint32_t offset)
    {
        LOG(INFO) << "mapped io_load: offset=" << offset;
        return mapped_io_load_.at(offset)();
    }

    /// @param offset
    /// @param value
    void io_store(const uint32_t offset, const uint16_t value)
    {
        LOG(INFO) << "mapped io_store: offset=" << offset
                  << ", value=" << value;
        return mapped_io_store_.at(offset)(value);
    }
};
} // namespace MPCE