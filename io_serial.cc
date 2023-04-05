#include <glog/logging.h>

#include "io_serial.h"

using namespace std;

/// @returns
uint16_t io_serial_interface_t::mmio_read()
{
    scoped_lock<mutex> lock(mutex_mmio_in_);

    const uint8_t value = mmio_in_buffer_.front();
    mmio_in_buffer_.pop();

    VLOG(0) << "io_serial read " << value << ", '" << static_cast<char>(value)
            << "'";

    return value;
}

/// @param byte
void io_serial_interface_t::mmio_write(const uint16_t byte)
{
    scoped_lock<mutex> lock(mutex_mmio_out_);

    VLOG(0) << "io_serial write " << byte << ", '" << static_cast<char>(byte)
            << "'";

    mmio_out_buffer_.push(byte);
}

/// @returns
uint16_t io_serial_interface_t::mmio_buffer_nonempty()
{
    bool in_buffer_nonempty;

    {
        scoped_lock<mutex> lock(mutex_mmio_in_);
        in_buffer_nonempty = mmio_in_buffer_.size();
    }

    return in_buffer_nonempty ? 1 : 0;
}

/// @param interrupt
void io_serial_interface_t::mmio_irq_notify(interrupt_t &interrupt)
{
    bool in_buffer_nonempty;

    {
        scoped_lock<mutex> lock(mutex_mmio_in_);
        in_buffer_nonempty = mmio_in_buffer_.size();
    }

    if (in_buffer_nonempty)
    {
        interrupt.signal(IRQ1);
    }
}

///
void io_serial_interface_t::start_console()
{
    running_ = true;

    // Console input thread, for reading in keystrokes and queueing them in
    // the MMIO.
    console_in_thread_ = thread(bind(&io_serial_interface_t::loop_in, this));

    // Console output thread, for writing output from the MMIO output
    // buffer.
    console_out_thread_ = thread(bind(&io_serial_interface_t::loop_out, this));
}

///
void io_serial_interface_t::join_console()
{
    console_in_thread_.join();
    console_out_thread_.join();
}

///
void io_serial_interface_t::stop_console()
{
    running_ = false;
}

///
void io_serial_interface_t::loop_out()
{
    while (running_)
    {
        optional<uint8_t> byte_out = nullopt;

        {
            scoped_lock<mutex> lock(mutex_mmio_out_);

            if (mmio_out_buffer_.size())
            {
                byte_out = mmio_out_buffer_.front();
                mmio_out_buffer_.pop();
            }
        }

        if (byte_out)
        {
            LOG(INFO) << static_cast<char>(*byte_out) << flush;
        }

        this_thread::sleep_for(sleep_duration_);
    }
}

///
void io_serial_interface_t::loop_in()
{
    while (running_)
    {
        uint8_t byte_in;

        /// Read a byte from stdin, including whitespace (noskipws).
        cin >> noskipws >> byte_in;

        {
            scoped_lock<mutex> lock(mutex_mmio_in_);
            mmio_in_buffer_.push(byte_in);
        }

        if (byte_in == 'Q')
            running_ = false;

        this_thread::sleep_for(sleep_duration_);
    }
}