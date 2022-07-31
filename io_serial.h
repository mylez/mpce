#pragma once

#include "interrupt.h"
#include "io.h"

#include <cstdint>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

namespace MPCE
{

using namespace std;

class IOSerialInterface : public IOInterface
{
  private:
    queue<uint8_t> mmio_out_buffer_;

    queue<uint8_t> mmio_in_buffer_;

    mutex mutex_mmio_in_;

    mutex mutex_mmio_out_;

    thread console_in_thread_;

    thread console_out_thread_;

    atomic<bool> running_;

    const chrono::milliseconds sleep_duration_{5};

  public:
    /// @returns
    uint16_t mmio_read()
    {
        scoped_lock<mutex> lock(mutex_mmio_in_);

        const uint8_t value = mmio_in_buffer_.front();
        mmio_in_buffer_.pop();

        printf("io_serial read %4x, '%c'\n", value, (char)value);

        return value;
    }

    /// @param byte
    void mmio_write(const uint16_t byte)
    {
        scoped_lock<mutex> lock(mutex_mmio_out_);

        printf("io_serial write %4x, '%c'\n", byte, (char)byte);

        mmio_out_buffer_.push(byte);
    }

    /// @returns
    uint16_t mmio_buffer_nonempty()
    {
        bool in_buffer_nonempty;

        {
            scoped_lock<mutex> lock(mutex_mmio_in_);
            in_buffer_nonempty = mmio_in_buffer_.size();
        }

        return in_buffer_nonempty ? 1 : 0;
    }

    /// @param interrupt
    void mmio_irq_notify(Interrupt &interrupt)
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
    void start_console()
    {
        running_ = true;

        // Console input thread, for reading in keystrokes and queueing them in
        // the MMIO.
        console_in_thread_ = thread(bind(&IOSerialInterface::loop_in, this));

        // Console output thread, for writing output from the MMIO output
        // buffer.
        console_out_thread_ = thread(bind(&IOSerialInterface::loop_out, this));
    }

    ///
    void join_console()
    {
        console_in_thread_.join();
        console_out_thread_.join();
    }

    ///
    void stop_console()
    {
        running_ = false;
    }

  private:
    ///
    void loop_out()
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
                cout << static_cast<char>(*byte_out);
            }

            this_thread::sleep_for(sleep_duration_);
        }
    }

    ///
    void loop_in()
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
};

} // namespace MPCE