#include "mmu.h"
#include "ram.h"

uint32_t mmu_t::resolve(const uint16_t virt_addr, uint8_t ptb,
                        const bool use_data_page_table, const bool is_write,
                        interrupt_t &interrupt)
{
    const ram_t &page_table =
        use_data_page_table ? page_table_data_ : page_table_code_;

    const uint32_t offset = VIRT_PAGE_OFFSET(virt_addr);
    const uint32_t page_number = VIRT_PAGE_NUM(virt_addr);
    const uint32_t pte_lookup_index = PTE_LOOKUP_INDEX(ptb, page_number, false);

    const uint16_t page_table_entry = page_table.load_w(pte_lookup_index);

    if (IS_PTE_UNMAPPED(page_table_entry))
    {
        interrupt.signal(PG_FAULT);
    }

    if (IS_PTE_READ_ONLY(page_table_entry) && is_write)
    {
        interrupt.signal(RO_FAULT);
    }

    return PHYS_ADDR(page_table_entry, offset);
}

ram_t &mmu_t::page_table(bool is_data)
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
bool mmu_t::read_only_fault()
{
    return read_only_fault_;
}

/// @returns True if a page fault has occurred.
bool mmu_t::page_fault()
{
    return page_fault_;
}

/// @returns True if any fault has occurred.
bool mmu_t::fault()
{
    return read_only_fault() || page_fault();
}

/// Reset fault flags to false.
void mmu_t::reset_fault()
{
    read_only_fault_ = false;
    page_fault_ = false;
}