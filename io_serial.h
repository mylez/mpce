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

class IOSerialInterface : public IOInterface
{
  private:
    std::queue<uint8_t> mmio_out_buffer_;

    std::queue<uint8_t> mmio_in_buffer_;

    std::mutex mutex_mmio_in_;

    std::mutex mutex_mmio_out_;

    std::optional<std::thread> console_in_thread_;

    std::optional<std::thread> console_out_thread_;

    std::atomic<bool> running_;

    const std::chrono::milliseconds sleep_duration_{5};

  public:
    uint16_t mmio_read()
    {
        std::scoped_lock<std::mutex> lock(mutex_mmio_in_);

        const uint8_t value = mmio_in_buffer_.front();
        mmio_in_buffer_.pop();

        return value;
    }

    void mmio_write(const uint16_t byte)
    {
        std::scoped_lock<std::mutex> lock(mutex_mmio_out_);

        mmio_out_buffer_.push(byte);
    }

    void mmio_irq_notify(Interrupt &interrupt)
    {
        bool in_buffer_nonempty;

        {
            std::scoped_lock<std::mutex> lock(mutex_mmio_in_);
            in_buffer_nonempty = mmio_in_buffer_.size();
        }

        if (in_buffer_nonempty)
        {
            interrupt.signal(IRQ1);
        }
    }

    uint16_t mmio_buffer_nonempty()
    {
        bool in_buffer_nonempty;

        {
            std::scoped_lock<std::mutex> lock(mutex_mmio_in_);
            in_buffer_nonempty = mmio_in_buffer_.size();
        }

        return in_buffer_nonempty ? 1 : 0;
    }

    void start_console()
    {
        running_ = true;

        // Console input thread, for reading in keystrokes and queueing them in
        // the MMIO.
        console_in_thread_ = std::thread([&]() {
            while (running_)
            {
                uint8_t byte_in;
                std::cin >> std::noskipws >> byte_in;

                {
                    std::scoped_lock<std::mutex> lock(mutex_mmio_in_);
                    mmio_in_buffer_.push(byte_in);

                    if (byte_in == 'Q')
                        running_ = false;
                }

                std::this_thread::sleep_for(sleep_duration_);
            }
        });

        // Console output thread, for writing output from the MMIO output
        // buffer.
        console_out_thread_ = std::thread([&]() {
            while (running_)
            {
                std::optional<uint8_t> byte_out = std::nullopt;

                {
                    std::scoped_lock<std::mutex> lock(mutex_mmio_out_);

                    if (mmio_out_buffer_.size())
                    {
                        byte_out = mmio_out_buffer_.front();
                        mmio_out_buffer_.pop();
                    }
                }

                if (byte_out)
                {
                    std::cout << static_cast<char>(*byte_out);
                }

                std::this_thread::sleep_for(sleep_duration_);
            }
        });
    }

    void join_console()
    {
        if (console_in_thread_)
        {
            console_in_thread_->join();
        }

        if (console_out_thread_)
        {
            console_out_thread_->join();
        }
    }

    void stop_console()
    {
        running_ = false;
    }
};

} // namespace MPCE