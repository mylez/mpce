#pragma once

#include <map>
#include <memory>
#include <vector>

#include <iostream>

using namespace std;

namespace MPCE
{

/// @brief
class WordAddressibleMemory
{
  private:
    std::unique_ptr<std::vector<uint16_t>> memory_;

    std::string name_;

  public:
    /// @param name
    /// @param capacity
    WordAddressibleMemory(std::string name, uint32_t capacity)
        : name_(name)
    {
        memory_ =
            std::make_unique<std::vector<uint16_t>>(capacity, 0);
        cout << "initializing memory " << name << " of size "
             << capacity << endl;
    }

    /// @param phys_addr
    uint16_t read(uint32_t phys_addr) const
    {
        printf("reading address 0x%x from memory %s\n", phys_addr,
               name_.c_str());
        return memory_->at(phys_addr);
    }

    /// @param phys_addr
    /// @param value
    void store(uint32_t phys_addr, uint16_t value)
    {
        memory_->at(phys_addr) = value;
    }

    /// @returns The number of words that this memory holds.
    uint32_t capacity() const
    {
        return memory_->capacity();
    }
};

/// @brief
class ByteAddressibleMemory
{
  private:
    /// @brief Word addressible base memory.
    WordAddressibleMemory memory_;

    /// @param entry
    static inline uint8_t get_high_byte(const uint16_t entry)
    {
        return (entry & 0xff00) >> 8;
    }

    /// @param entry
    static inline uint8_t get_low_byte(const uint16_t entry)
    {
        return entry & 0xff;
    }

    /// @param entry
    inline uint16_t get_word(const uint32_t phys_addr) const
    {
        return memory_.read(phys_addr / 2);
    }

    inline void set_word(const uint32_t phys_addr,
                         const uint16_t value)
    {
        memory_.store(phys_addr / 2, value);
    }

  public:
    /// @param name
    /// @param capacity
    ByteAddressibleMemory(std::string name, uint32_t capacity)
        : memory_{name, capacity}
    {
    }

    /// @param phys_addr
    uint16_t read_word(const uint32_t phys_addr) const
    {
        if (phys_addr >= capacity())
        {
            // Error condition.
            return 0;
        }

        return get_word(phys_addr);
    }

    /// @param phys_addr
    uint8_t read_byte(const uint32_t phys_addr) const
    {
        // read_word checks bounds.
        const uint16_t entry = get_word(phys_addr);
        const uint8_t byte_offset = phys_addr % 2;

        return byte_offset ? get_high_byte(entry)
                           : get_low_byte(entry);
    }

    /// @param phys_addr
    /// @param woru
    void store_word(const uint32_t phys_addr, const uint16_t word)
    {
        set_word(phys_addr, word);
    }

    /// @param phys_addr
    /// @param value
    void store_byte(const uint32_t phys_addr, const uint8_t value)
    {
        const uint16_t entry = get_word(phys_addr);
        const uint16_t word = phys_addr % 2
                                  ? (entry & 0x00ff) | (value << 8)
                                  : (entry & 0xff00) | (value & 0xff);

        set_word(phys_addr, word);
    }

    /// @returns Memory capacity in words.
    uint32_t capacity() const
    {
        return memory_.capacity();
    }
};

} // namespace MPCE