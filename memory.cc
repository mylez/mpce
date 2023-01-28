#include "memory.h"

namespace mpce
{
using namespace std;

/// @param mapped_io_begin
/// @param mapped_io_load
/// @param mapped_io_store
void memory_t::map_io(uint32_t mapped_io_begin,
                      function<uint16_t(uint32_t)> mapped_io_load,
                      function<void(uint32_t, uint16_t)> mapped_io_store)
{
    mapped_io_begin_ = mapped_io_begin;
    mapped_io_load_ = mapped_io_load;
    mapped_io_store_ = mapped_io_store;
}

/// @param name
/// @param capacity
word_addressible_memory_t::word_addressible_memory_t(const string name,
                                                     const uint32_t capacity)
    : memory_(capacity, 0), name_(name)
{
}

/// @param name
/// @param capacity
byte_addressible_memory_t::byte_addressible_memory_t(string name,
                                                     uint32_t capacity)
    : memory_{name, capacity}
{
}

/// @param phys_addr
uint16_t byte_addressible_memory_t::load_word(const uint32_t phys_addr) const
{
    if (phys_addr >= capacity())
    {
        // Error condition.
        return 0;
    }

    return get_word(phys_addr);
}

/// @param phys_addr
uint8_t byte_addressible_memory_t::load_byte(const uint32_t phys_addr) const
{
    const uint16_t entry = get_word(phys_addr);
    const uint8_t byte_offset = phys_addr % 2;

    return byte_offset ? high_byte(entry) : low_byte(entry);
}

/// @param phys_addr
/// @param woru
void byte_addressible_memory_t::store_word(const uint32_t phys_addr,
                                           const uint16_t word)
{
    set_word(phys_addr, word);
}

/// @param phys_addr
/// @param value
void byte_addressible_memory_t::store_byte(const uint32_t phys_addr,
                                           const uint8_t value)
{
    const uint16_t entry = get_word(phys_addr);
    const uint16_t word = phys_addr % 2 ? (entry & 0x00ff) | (value << 8)
                                        : (entry & 0xff00) | (value & 0xff);

    set_word(phys_addr, word);
}

/// @param entry
uint8_t byte_addressible_memory_t::high_byte(const uint16_t entry)
{
    return (entry & 0xff00) >> 8 + 0;
}

/// @param entry
uint8_t byte_addressible_memory_t::low_byte(const uint16_t entry)
{
    return entry & 0xff;
}

/// @param entry
uint16_t byte_addressible_memory_t::get_word(const uint32_t phys_addr) const
{
    return memory_.load(phys_addr >> 1);
}

/// @param phys_addr
/// @param value
void byte_addressible_memory_t::set_word(const uint32_t phys_addr,
                                         const uint16_t value)
{
    memory_.store(phys_addr >> 1, value);
}

}; // namespace mpce