#pragma once

#include "interrupt.h"
#include "io_serial.h"
#include "memory.h"

#include <functional>

class io_interface_t
{
  public:
    virtual uint16_t mmio_read() = 0;

    virtual void mmio_write(const uint16_t) = 0;

    virtual void mmio_irq_notify(interrupt_t &) = 0;
};
