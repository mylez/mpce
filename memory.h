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
  private:
    ///
    std::optional<std::function<uint16_t(uint32_t)>> mapped_io_load_;

    ///
    std::optional<std::function<void(uint32_t, uint16_t)>> mapped_io_store_;

    ///
    uint32_t mapped_io_begin_;

  public:
    /// @param phys_addr
    /// @param byte
    ///@returns description of the return value
    virtual uint16_t load(uint32_t phys_addr, bool byte = false) const
    {
        if (mapped_io_load_ && phys_addr > mapped_io_begin_)
        {
            return (*mapped_io_load_)(phys_addr - mapped_io_begin_ - 1);
        }

        return do_load(phys_addr, byte);
    }

    /// @param phys_addr
    /// @param value
    /// @param byte
    virtual void store(uint32_t phys_addr, uint16_t value, bool byte = false)
    {
        if (mapped_io_store_ && phys_addr > mapped_io_begin_)
        {
            (*mapped_io_store_)(phys_addr - mapped_io_begin_ - 1, value);
            return;
        }

        do_store(phys_addr, value, byte);
    };

    /// @returns
    virtual uint32_t capacity() const = 0;

    /// @param mapped_io_begin
    /// @param mapped_io_load
    /// @param mapped_io_store
    void map_io(uint32_t mapped_io_begin,
                std::function<uint16_t(uint32_t)> mapped_io_load,
                std::function<void(uint32_t, uint16_t)> mapped_io_store)
    {
        mapped_io_begin_ = mapped_io_begin;
        mapped_io_load_ = mapped_io_load;
        mapped_io_store_ = mapped_io_store;
    }

  protected:
    virtual void do_store(uint32_t, uint16_t, bool) = 0;

    virtual uint16_t do_load(uint32_t, bool) const = 0;
};

class WordAddressibleMemory : public Memory
{
  private:
    ///
    std::unique_ptr<std::vector<uint16_t>> memory_;

    ///
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

    /// @returns The number of words that this memory holds.
    uint32_t capacity() const override
    {
        return memory_->capacity();
    }

  protected:
    /// @param phys_addr
    /// @param byte
    uint16_t do_load(uint32_t phys_addr, bool byte = false) const override
    {
        printf("mem %s: loading %s from addr 0x%x\n", name_.c_str(),
               byte ? "byte" : "word", phys_addr);
        return memory_->at(phys_addr);
    }

    /// @param phys_addr
    /// @param value
    void do_store(uint32_t phys_addr, uint16_t value,
                  bool byte = false) override
    {
        printf("mem %s: storing %s 0x%x to addr 0x%x\n", name_.c_str(),
               byte ? "byte" : "word", value, phys_addr);
        memory_->at(phys_addr) = value;
    }
};

class ByteAddressibleMemory : public Memory
{
  private:
    /// Todo: The state in memory_ should be inherited from Memory by both
    /// WordAddressibleMemory and ByteAddressibleMemory.

    /// Word addressible base memory.
    WordAddressibleMemory memory_;

  public:
    /// @param name
    /// @param capacity
    ByteAddressibleMemory(std::string name, uint32_t capacity)
        : memory_{name, capacity}
    {
    }

    uint16_t do_load(const uint32_t phys_addr, bool byte) const override
    {
        return byte ? load_byte(phys_addr) : load_word(phys_addr);
    }

    void do_store(const uint32_t phys_addr, uint16_t value, bool byte) override
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

    /// @param phys_addr
    /// @param value
    void set_word(const uint32_t phys_addr, const uint16_t value)
    {
        memory_.store(phys_addr >> 1, value);
    }
};

} // namespace MPCE