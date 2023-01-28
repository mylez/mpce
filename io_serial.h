#pragma once

#include "interrupt.h"
#include "io.h"

#include <atomic>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

namespace mpce
{

using namespace std;

class io_serial_interface_t : public io_interface_t
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
    uint16_t mmio_read();

    /// @param byte
    void mmio_write(const uint16_t byte);

    /// @returns
    uint16_t mmio_buffer_nonempty();

    /// @param interrupt
    void mmio_irq_notify(interrupt_t &interrupt);

    ///
    void start_console();

    ///
    void join_console();

    ///
    void stop_console();

  private:
    ///
    void loop_out();

    ///
    void loop_in();
};

} // namespace mpce