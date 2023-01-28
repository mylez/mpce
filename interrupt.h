#pragma once

#include <cstdint>
#include <mutex>
#include <unordered_set>
#include <vector>

namespace mpce
{

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
    /// @brief
    /// @return
    uint8_t cause();

    void signal(const interrupt_signal_t signal);

    /// @brief
    /// @param signals
    /// @return
    bool is_signalled(const vector<interrupt_signal_t> signals);

    /// @brief
    void clear();

  private:
    /// @brief
    /// @param byte
    /// @param bit
    /// @param signal
    void set_bit_if_signalled(uint8_t &byte, const uint8_t bit,
                              const interrupt_signal_t signal) const;

    /// @brief
    /// @param byte
    void set_signal_priority(uint8_t &byte) const;

    /// @brief
    unordered_set<interrupt_signal_t> pending_interrupts_;

    /// @brief
    mutex mutex_;
};

} // namespace mpce