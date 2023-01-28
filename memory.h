#pragma once

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

#include <glog/logging.h>

using namespace std;

namespace mpce
{

class memory_t
{
  private:
    ///
    optional<function<uint16_t(uint32_t)>> mapped_io_load_;

    ///
    optional<function<void(uint32_t, uint16_t)>> mapped_io_store_;

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
            LOG(INFO) << "rerouting load to io, phys_addr=" << phys_addr;
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
            LOG(INFO) << "rerouting store to io, phys_addr=" << phys_addr;
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
                function<uint16_t(uint32_t)> mapped_io_load,
                function<void(uint32_t, uint16_t)> mapped_io_store);

  protected:
    virtual void do_store(uint32_t, uint16_t, bool) = 0;

    virtual uint16_t do_load(uint32_t, bool) const = 0;
};

class word_addressible_memory_t : public memory_t
{
  private:
    ///
    vector<uint16_t> memory_;

    ///
    string name_;

  public:
    /// @param name
    /// @param capacity
    word_addressible_memory_t(const string name, const uint32_t capacity);

    /// @returns The number of words that this memory holds.
    uint32_t capacity() const override
    {
        return memory_.capacity();
    }

  protected:
    /// @param phys_addr
    /// @param byte
    uint16_t do_load(uint32_t phys_addr, bool byte = false) const override
    {
        LOG(INFO) << "mem " << name_ << ": loading " << (byte ? "byte" : "word")
                  << " from addr " << phys_addr;

        return memory_.at(phys_addr);
    }

    /// @param phys_addr
    /// @param value
    void do_store(uint32_t phys_addr, uint16_t value,
                  bool byte = false) override
    {
        LOG(INFO) << "mem " << name_ << ": storing " << (byte ? "byte" : "word")
                  << " " << value << " to addr " << phys_addr;

        memory_.at(phys_addr) = value;
    }
};

class byte_addressible_memory_t : public memory_t
{
  private:
    /// Todo: The state in memory_ should be inherited from memory_t by both
    /// word_addressible_memory_t and byte_addressible_memory_t.

    /// Word addressible base memory.
    word_addressible_memory_t memory_;

  public:
    /// @param name
    /// @param capacity
    byte_addressible_memory_t(string name, uint32_t capacity);

    /// @returns memory_t capacity in words.
    uint32_t capacity() const override
    {
        return memory_.capacity();
    }

  protected:
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

  private:
    /// @param phys_addr
    uint16_t load_word(const uint32_t phys_addr) const;

    /// @param phys_addr
    uint8_t load_byte(const uint32_t phys_addr) const;

    /// @param phys_addr
    /// @param woru
    void store_word(const uint32_t phys_addr, const uint16_t word);

    /// @param phys_addr
    /// @param value
    void store_byte(const uint32_t phys_addr, const uint8_t value);

    /// @param entry
    static inline uint8_t high_byte(const uint16_t entry);

    /// @param entry
    static inline uint8_t low_byte(const uint16_t entry);

    /// @param entry
    uint16_t get_word(const uint32_t phys_addr) const;

    /// @param phys_addr
    /// @param value
    void set_word(const uint32_t phys_addr, const uint16_t value);
};

} // namespace mpce