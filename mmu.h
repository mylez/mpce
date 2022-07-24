#pragma once

#include "memory.h"
#include "register.h"

#define VIRT_PAGE_NUM(a) (static_cast<uint32_t>(((a)&0xfe00) >> 9))
#define VIRT_PAGE_OFFSET(a) (static_cast<uint32_t>((a)&0x01ff))
#define PHYS_ADDR(pte, offset)                                                 \
    ((static_cast<uint32_t>((pte)&0x1fff) << 14) | (offset)&0x01ff)
#define PTE_LOOKUP_INDEX(ptb, page_num, io_data)                               \
    (static_cast<uint32_t>(((ptb) << 7) | (page_num) | (io_data ? 0x8000 : 0)))
#define IS_PTE_READ_ONLY(pte) (static_cast<bool>(pte & 0x4000))
#define IS_PTE_UNMAPPED(pte) (static_cast<bool>((pte)&0x8000))

namespace MPCE
{

class MMU
{
  private:
    WordAddressibleMemory page_table_code_{"page_table_code", 0x1'0000};

    WordAddressibleMemory page_table_data_{"page_table_data", 0x1'0000};

    bool read_only_fault_{false};

    bool page_fault_{false};

  public:
    uint32_t resolve(const uint16_t virt_addr, uint8_t ptb,
                     const bool use_data_page_table,
                     const bool perform_fault_check, const bool is_write)
    {
        /// Todo: figure out what this was for.
        const bool io_data = false;

        /// Todo: resolve by PTE.
        const WordAddressibleMemory &page_table =
            use_data_page_table ? page_table_data_ : page_table_code_;

        const uint32_t offset = VIRT_PAGE_OFFSET(virt_addr);
        const uint32_t page_number = VIRT_PAGE_NUM(virt_addr);
        const uint32_t pte_lookup_index =
            PTE_LOOKUP_INDEX(ptb, page_number, io_data);

        const uint16_t page_table_entry = page_table.load(pte_lookup_index);

        if (perform_fault_check)
        {
            page_fault_ = IS_PTE_UNMAPPED(page_table_entry);
            read_only_fault_ = IS_PTE_READ_ONLY(page_table_entry) && is_write;
        }

        return PHYS_ADDR(page_table_entry, offset);
    }

    WordAddressibleMemory &page_table(bool is_data)
    {
        if (is_data)
        {
            return page_table_data_;
        }
        else
        {
            return page_table_code_;
        }
    }

    /// @returns True if a read only fault has occurred.
    bool read_only_fault()
    {
        return read_only_fault_;
    }

    /// @returns True if a page fault has occurred.
    bool page_fault()
    {
        return page_fault_;
    }

    /// @returns True if any fault has occurred.
    bool fault()
    {
        return read_only_fault() || page_fault();
    }

    /// Reset fault flags to false.
    void reset_fault()
    {
        read_only_fault_ = false;
        page_fault_ = false;
    }
};

} // namespace MPCE