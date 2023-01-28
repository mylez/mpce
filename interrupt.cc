#include "interrupt.h"

#pragma once

#include <cstdint>
#include <mutex>
#include <unordered_set>
#include <vector>

namespace mpce
{

using namespace std;

uint8_t interrupt_t::cause()
{
    uint8_t status_byte = 0;

    scoped_lock<mutex> lock(mutex_);

    set_bit_if_signalled(status_byte, 0, IRQ0);
    set_bit_if_signalled(status_byte, 1, IRQ1);
    set_bit_if_signalled(status_byte, 2, IRQ2);
    set_bit_if_signalled(status_byte, 3, IRQ3);

    set_signal_priority(status_byte);

    return status_byte;
}

void interrupt_t::signal(const interrupt_signal_t signal)
{
    scoped_lock<mutex> lock(mutex_);

    pending_interrupts_.insert(signal);
}

/// @brief
/// @param signals
/// @return
bool interrupt_t::is_signalled(const vector<interrupt_signal_t> signals)
{
    scoped_lock<mutex> lock(mutex_);

    for (const interrupt_signal_t signal : signals)
    {
        if (pending_interrupts_.count(signal))
        {
            return true;
        }
    }

    return false;
}

/// @brief
void interrupt_t::clear()
{
    scoped_lock<mutex> lock(mutex_);

    pending_interrupts_.clear();
}

/// @brief
/// @param byte
/// @param bit
/// @param signal
void interrupt_t::set_bit_if_signalled(uint8_t &byte, const uint8_t bit,
                                     const interrupt_signal_t signal) const
{
    if (pending_interrupts_.count(signal))
    {
        byte |= 1 << bit;
    }
}

/// @brief
/// @param byte
void interrupt_t::set_signal_priority(uint8_t &byte) const
{
    uint8_t priority = 0;
    uint8_t i = 1;

    for (const interrupt_signal_t signal :
         {TIME_OUT, RO_FAULT, PG_FAULT, ILL_INST})
    {
        if (pending_interrupts_.count(signal))
        {
            priority = i;
        }

        i++;
    }

    byte |= priority << 4;
}

}; // namespace mpce