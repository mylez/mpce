#pragma once

#include <cstdint>
#include <mutex>
#include <unordered_set>
#include <vector>

using namespace std;

enum interrupt_signal_t
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

class interrupt_t
{
  public:
    ///
    /// @return
    uint8_t cause();

    void signal(const interrupt_signal_t signal);

    ///
    /// @param signals
    /// @return
    bool is_signalled(const vector<interrupt_signal_t> signals);

    ///
    void clear();

  private:
    ///
    /// @param byte
    /// @param bit
    /// @param signal
    void set_bit_if_signalled(uint8_t &byte, const uint8_t bit,
                              const interrupt_signal_t signal) const;

    ///
    /// @param byte
    void set_signal_priority(uint8_t &byte) const;

    ///
    unordered_set<interrupt_signal_t> pending_interrupts_;

    ///
    mutex mutex_;
};