#pragma once

#include "interrupt.h"
#include "ram.h"

#define VIRT_PAGE_NUM(a) (static_cast<uint32_t>(((a)&0xfe00) >> 9))
#define VIRT_PAGE_OFFSET(a) (static_cast<uint32_t>((a)&0x01ff))
#define PHYS_ADDR(pte, offset)                                                 \
    ((static_cast<uint32_t>((pte)&0x1fff) << 14) | ((offset)&0x01ff))
#define PTE_LOOKUP_INDEX(ptb, page_num, io_data)                               \
    (static_cast<uint32_t>(((ptb) << 7) | (page_num) | (io_data ? 0x8000 : 0)))
#define IS_PTE_READ_ONLY(pte) (static_cast<bool>(pte & 0x4000))
#define IS_PTE_UNMAPPED(pte) (static_cast<bool>((pte)&0x8000))

class mmu_t
{
  private:
    const uint32_t mmu_capacity_ = 0x2'0000;

    ram_t page_table_code_;

    ram_t page_table_data_;

    bool read_only_fault_ = false;

    bool page_fault_ = false;

  public:
    mmu_t()
        : page_table_code_(mmu_capacity_),
          page_table_data_(mmu_capacity_)
    {
    }
    /// @param virt_addr
    /// @param ptb
    /// @param use_data_page_table
    /// @param is_write
    /// @param interrupt
    /// @return
    uint32_t resolve(const uint16_t virt_addr, uint8_t ptb,
                     const bool use_data_page_table, const bool is_write,
                     interrupt_t &interrupt);

    /// @param is_data
    /// @return
    ram_t &page_table(bool is_data);

    /// @returns True if a read only fault has occurred.
    bool read_only_fault();

    /// @returns True if a page fault has occurred.
    bool page_fault();

    /// @returns True if any fault has occurred.
    bool fault();

    /// Reset fault flags to false.
    void reset_fault();
};
