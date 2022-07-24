#pragma once

#include <map>
#include <memory>
#include <vector>

#include <iostream>

using namespace std;

namespace MPCE
{

class Memory
{
  public:
    /// @brief
    virtual uint16_t load(uint32_t phys_addr, bool byte = false) const = 0;

    /// @brief
    virtual void store(uint32_t phys_addr, uint16_t value,
                       bool byte = false) = 0;

    /// @brief
    virtual uint32_t capacity() const = 0;
};

/// @brief
class WordAddressibleMemory : public Memory
{
  private:
    std::unique_ptr<std::vector<uint16_t>> memory_;

    std::string name_;

  public:
    /// @param name
    /// @param capacity
    WordAddressibleMemory(std::string name, uint32_t capacity) : name_(name)
    {
        memory_ = std::make_unique<std::vector<uint16_t>>(capacity, 0);
        cout << "initializing memory " << name << " of size " << capacity
             << endl;
    }

    /// @param phys_addr
    uint16_t load(uint32_t phys_addr, bool byte = false) const override
    {
        printf("mem %s: loading %s from addr 0x%x\n", name_.c_str(),
               byte ? "byte" : "word", phys_addr);
        return memory_->at(phys_addr);
    }

    /// @param phys_addr
    /// @param value
    void store(uint32_t phys_addr, uint16_t value, bool byte = false) override
    {
        printf("mem %s: storing %s 0x%x to addr 0x%x\n", name_.c_str(),
               byte ? "byte" : "word", value, phys_addr);
        memory_->at(phys_addr) = value;
    }

    /// @returns The number of words that this memory holds.
    uint32_t capacity() const override
    {
        return memory_->capacity();
    }
};

/// @brief
class ByteAddressibleMemory : public Memory
{
  private:
    /// Todo: The state in memory_ should be inherited from Memory by both
    /// WordAddressibleMemory and ByteAddressibleMemory.
    /// @brief Word addressible base memory.
    WordAddressibleMemory memory_;

  public:
    /// @param name
    /// @param capacity
    ByteAddressibleMemory(std::string name, uint32_t capacity)
        : memory_{name, capacity}
    {
    }

    uint16_t load(const uint32_t phys_addr, bool byte) const override
    {
        return byte ? load_byte(phys_addr) : load_word(phys_addr);
    }

    void store(const uint32_t phys_addr, uint16_t value, bool byte) override
    {
        if (byte)
        {
            store_byte(phys_addr, value);
        }
        else
        {
            store_word(phys_addr, value);
        }
    }

    /// @returns Memory capacity in words.
    uint32_t capacity() const override
    {
        return memory_.capacity();
    }

  private:
    /// @param phys_addr
    uint16_t load_word(const uint32_t phys_addr) const
    {
        if (phys_addr >= capacity())
        {
            // Error condition.
            return 0;
        }

        return get_word(phys_addr);
    }

    /// @param phys_addr
    uint8_t load_byte(const uint32_t phys_addr) const
    {
        const uint16_t entry = get_word(phys_addr);
        const uint8_t byte_offset = phys_addr % 2;

        return byte_offset ? high_byte(entry) : low_byte(entry);
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
        const uint16_t word = phys_addr % 2 ? (entry & 0x00ff) | (value << 8)
                                            : (entry & 0xff00) | (value & 0xff);

        set_word(phys_addr, word);
    }

    /// @param entry
    static inline uint8_t high_byte(const uint16_t entry)
    {
        return (entry & 0xff00) >> 8;
    }

    /// @param entry
    static inline uint8_t low_byte(const uint16_t entry)
    {
        return entry & 0xff;
    }

    /// @param entry
    uint16_t get_word(const uint32_t phys_addr) const
    {
        return memory_.load(phys_addr >> 1);
    }

    void set_word(const uint32_t phys_addr, const uint16_t value)
    {
        memory_.store(phys_addr >> 1, value);
    }
};

} // namespace MPCE