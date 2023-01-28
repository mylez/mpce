#include "mmio.h"

namespace mpce
{

///
MMIO::MMIO()
    : mapped_io_load_{mapped_io_size_}, mapped_io_store_{mapped_io_size_}
{
    using namespace placeholders;

    LOG(INFO) << "initializing mmio\n";

    kern_data_.map_io(mapped_io_begin_, bind(&MMIO::io_load, this, _1),
                      bind(&MMIO::io_store, this, _1, _2));

    /// register_t the mapped IO handlers for the serial console.

    mapped_io_load_.at(0x00) =
        bind(&io_serial_interface_t::mmio_read, &serial_interface_);

    mapped_io_store_.at(0x00) =
        bind(&io_serial_interface_t::mmio_write, &serial_interface_, _1);

    mapped_io_store_.at(0x01) =
        bind(&io_serial_interface_t::mmio_buffer_nonempty, &serial_interface_);

    irq_notifiers_.push_back(
        bind(&io_serial_interface_t::mmio_irq_notify, &serial_interface_, _1));
}

/// @param is_user_mode
memory_t &MMIO::get_code(bool is_user_mode)
{
    return is_user_mode ? user_code_ : kern_code_;
}

/// @param is_user_mode
memory_t &MMIO::get_data(bool is_user_mode)
{
    return is_user_mode ? user_data_ : kern_data_;
}

/// @returns
io_serial_interface_t &MMIO::serial_interface()
{
    return serial_interface_;
}

/// @param interrupt
void MMIO::irq_notify(interrupt_t &interrupt)
{
    for (auto notify : irq_notifiers_)
    {
        notify(interrupt);
    }
}

/// @param offset
/// @returns
uint16_t MMIO::io_load(const uint32_t offset)
{
    LOG(INFO) << "mapped io_load: offset=" << offset;
    return mapped_io_load_.at(offset)();
}

/// @param offset
/// @param value
void MMIO::io_store(const uint32_t offset, const uint16_t value)
{
    LOG(INFO) << "mapped io_store: offset=" << offset << ", value=" << value;
    return mapped_io_store_.at(offset)(value);
}

}; // namespace mpce