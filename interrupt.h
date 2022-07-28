#pragma once

#include <cstdint>
#include <unordered_set>
#include <vector>

namespace MPCE
{
enum InterruptSignal
{
    IRQ0,
    IRQ1,
    IRQ2,
    IRQ3,
    TIME_OUT,
    RO_FAULT,
    PG_FAULT,
    ILL_INST
};

class Interrupt
{
  public:
    uint8_t cause() const
    {
        uint8_t status_byte = 0;

        set_bit_if_signalled(status_byte, 0, IRQ0);
        set_bit_if_signalled(status_byte, 1, IRQ1);
        set_bit_if_signalled(status_byte, 2, IRQ2);
        set_bit_if_signalled(status_byte, 3, IRQ3);

        set_signal_priority(status_byte);

        return status_byte;
    }

    void signal(const InterruptSignal signal)
    {
        pending_interrupts_.insert(signal);
    }

    bool is_signalled(const std::vector<InterruptSignal> signals) const
    {
        for (const InterruptSignal signal : signals)
        {
            if (pending_interrupts_.count(signal))
            {
                return true;
            }
        }

        return false;
    }

    void clear()
    {
        pending_interrupts_.clear();
    }

  private:
    void set_bit_if_signalled(uint8_t &byte, const uint8_t bit,
                              const InterruptSignal signal) const
    {
        if (pending_interrupts_.count(signal))
        {
            byte |= 1 << bit;
        }
    }

    void set_signal_priority(uint8_t &byte) const
    {
        uint8_t priority = 0;
        uint8_t i = 1;

        for (const InterruptSignal signal :
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

    std::unordered_set<InterruptSignal> pending_interrupts_;
};

} // namespace MPCE