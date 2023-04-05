#pragma once

#include "interrupt.h"
#include "memory.h"
#include "mmio.h"
#include "mmu.h"
#include "register.h"

#include <cstdint>
#include <functional>

#define OPCODE(inst) (((inst) >> 9) & 0x007f)
#define REG_SEL_X(inst) ((inst)&0x0007)
#define REG_SEL_Y(inst) (((inst) >> 3) & 0x0007)
#define REG_SEL_Z(inst) (((inst) >> 6) & 0x0007)
#define BIND_OP(op) (bind((op), this))
#define MAP_OPCODE(opcode, op) opcode_mapping_.at((opcode) >> 1) = (op)

#define STATUS_NEGATIVE 0x08
#define STATUS_ZERO 0x04
#define STATUS_CARRY 0x02
#define STATUS_OVERFLOW 0x02

#define OPCODE_MAP_SIZE 0x80

using namespace std;

struct cpu_state_t
{
  private:
    using Operation = function<void()>;

    /// Map a 7-bit opcode to one of 128 Operation functions. Initialize entries
    /// to op_invalid.
    vector<Operation> opcode_mapping_{OPCODE_MAP_SIZE,
                                      BIND_OP(&cpu_state_t::op_invalid)};

    /// Main architectural registers, including program counter.
    reg_file_t register_file_;

    /// memory management unit, maps virtual address to physical address in
    /// user mode.
    mmu_t mmu_;

    /// memory-mapped IO.
    MMIO mmio_;

    /// Special registers.
    reg_t<uint8_t> status_{"status", 0xf0};
    reg_t<uint8_t> cause_{"cause"};
    reg_t<uint16_t> eret_{"eret"};
    reg_t<uint16_t> context_{"context"};
    reg_t<uint16_t> timer_{"timer"};
    reg_t<uint16_t> isr_{"isr"};
    reg_t<uint16_t> ptb_{"ptb"};
    reg_t<uint16_t> exc_addr_{"exc_addr"};
    reg_t<uint16_t> inst_{"inst"};
    reg_t<uint8_t> mode_{"mode", 0xfe};

    interrupt_t interrupt_;

  public:
    cpu_state_t();

    void cycle();

    MMIO &mmio();

  private:
    ///
    /// @param signals
    void context_switch_to_isr_if(const vector<interrupt_signal_t> signals);

    /// @param reg_x
    void load_inst_word(reg_t<uint16_t> &reg_x);

    /// Enable user mode by setting the mode register to 1.
    void op_set_mode();

    /// Atomic test and set.
    void op_ats();

    ///
    void op_invalid();

    ///
    void op_none();

    ///
    bool is_user_mode() const;

    /// @tparam is_data
    template <bool is_data> void op_store_page_table_entry()
    {
        if (is_user_mode())
        {
            interrupt_.signal(ILL_INST);
            return;
        }

        const uint16_t inst_word = inst_.read();

        const uint16_t x = register_file_.get(REG_SEL_X(inst_word)).read();
        const uint16_t y = register_file_.get(REG_SEL_Y(inst_word)).read();
        const uint16_t z = register_file_.get(REG_SEL_Z(inst_word)).read();

        const uint16_t phys_addr = y + z;

        mmu_.page_table(is_data).store_w(phys_addr, x);
    }

    /// @tparam protected_inst
    /// @tparam load_imm
    /// @tparam toggle_mode
    /// @tparam reg_type_t
    /// @param special_reg
    /// @returns An Operation object that can be called like a function.
    template <bool protected_inst, bool load_imm, bool toggle_mode,
              typename reg_type_t>
    Operation op_special_reg_read(const reg_t<reg_type_t> &special_reg)
    {
        return [&]() {
            if (load_imm)
            {
                load_inst_word(register_file_.get(IMM));
            }

            if (interrupt_.is_signalled({PG_FAULT}))
            {
                return;
            }
            else if (protected_inst && is_user_mode())
            {
                interrupt_.signal(ILL_INST);
                return;
            }

            const uint16_t inst_word = inst_.read();

            reg_t<uint16_t> &reg_x = register_file_.get(REG_SEL_X(inst_word));

            reg_x.write(special_reg.read());

            if (toggle_mode)
            {
                mode_.write(mode_.read() ^ 1);
            }
        };
    }

    /// @tparam load_imm
    /// @tparam reg_type_t
    /// @param special_reg
    template <bool load_imm, typename reg_type_t>
    Operation op_special_reg_write(reg_t<reg_type_t> &special_reg)
    {
        return [&]() {
            if (is_user_mode())
            {
                interrupt_.signal(ILL_INST);
                return;
            }

            if (load_imm)
            {
                load_inst_word(register_file_.get(IMM));

                if (interrupt_.is_signalled({PG_FAULT}))
                {
                    return;
                }
            }

            const uint16_t inst_word = inst_.read();
            const uint16_t y = register_file_.get(REG_SEL_Y(inst_word)).read();
            const uint16_t z = register_file_.get(REG_SEL_Z(inst_word)).read();

            const uint16_t value = y + z;

            special_reg.write(value);
        };
    }

    /// @tparam alu_sel
    /// @tparam carry_in
    /// @tparam imm
    /// @tparam cond A bit mask for the status register encoding the required
    /// condition for inst to execute.
    /// @tparam status_invert Whether to consider the inverse of the condition.
    /// @tparam toggle_mode Whether to switch from mode to !mode.
    template <uint32_t alu_sel, bool carry_in, bool load_imm,
              uint8_t cond = 0xff, bool status_invert = false,
              bool toggle_mode = false>
    void op_alu()
    {
        const bool user_mode = is_user_mode();

        LOG(INFO) << "op_alu";

        if (user_mode && toggle_mode)
        {
            interrupt_.signal(ILL_INST);
            return;
        }

        if (load_imm)
        {
            load_inst_word(register_file_.get(IMM));

            if (user_mode && interrupt_.is_signalled({PG_FAULT}))
            {
                return;
            }
        }

        // Do not proceed with this operation if condition is not satisfied.
        if (static_cast<bool>(cond & status_.read()) != status_invert)
        {
            return;
        }

        const uint16_t inst_word = inst_.read();

        const uint16_t y = register_file_.get(REG_SEL_Y(inst_word)).read();
        const uint16_t z = register_file_.get(REG_SEL_Z(inst_word)).read();

        reg_t<uint16_t> &reg_x = register_file_.get(REG_SEL_X(inst_word));

        uint16_t x = 0;

        bool update_status = false;
        bool update_status_unsigned = false;
        // bool carry_out = false;
        // bool overflow = false;
        bool zero = false;
        bool negative = false;

        // Todo: Add all ALU operations and status checks.
        switch (alu_sel)
        {
        case 0:
            // Addition.
            x = static_cast<uint16_t>(static_cast<int16_t>(y) +
                                      static_cast<int16_t>(z));
            update_status = true;
            break;

        default:
            // Subtraction.
            x = static_cast<uint16_t>(static_cast<int16_t>(y) -
                                      static_cast<int16_t>(z));
            update_status = true;
            break;
        }

        // Set the destination register here.
        reg_x.write(x);

        if (update_status)
        {
            negative = static_cast<int16_t>(z) < 0;
        }

        if (update_status || update_status_unsigned)
        {
            zero = z == 0;
            // Todo: Set carry_out, overflow.
        }

        if (toggle_mode)
        {
            mode_.write(mode_.read() ^ 1);
        }
    }

    /// @tparam byte
    /// @tparam mode
    /// @tparam is_data
    /// @tparam is_store
    /// @tparam load_imm
    /// @tparam sign_extend_byte
    template <bool byte, bool inst_mode, bool is_data, bool is_store,
              bool load_imm, bool sign_extend_byte>
    void op_mem()
    {
        const bool user_mode = is_user_mode();

        LOG(INFO) << "mem data byte=" << byte << " mode=" << inst_mode
                  << " data=" << is_data << " store=" << is_store
                  << " imm=" << load_imm << " extend=" << sign_extend_byte
                  << "\n";

        if (user_mode && !inst_mode)
        {
            interrupt_.signal(ILL_INST);
            return;
        }

        if (load_imm)
        {
            load_inst_word(register_file_.get(IMM));
        }

        const uint16_t inst_word = inst_.read();

        reg_t<uint16_t> &reg_x = register_file_.get(REG_SEL_X(inst_word));

        const uint16_t y = register_file_.get(REG_SEL_Y(inst_word)).read();
        const uint16_t z = register_file_.get(REG_SEL_Z(inst_word)).read();

        mmu_.reset_fault();

        const uint16_t virt_addr = y + z;
        const uint32_t phys_addr =
            user_mode ? mmu_.resolve(virt_addr, ptb_.read(), is_data, is_store,
                                     interrupt_)
                      : virt_addr;

        if (interrupt_.is_signalled({PG_FAULT, RO_FAULT}))
        {
            return;
        }

        ram_t &memory =
            is_data ? mmio_.get_data(inst_mode) : mmio_.get_code(inst_mode);

        if (is_store)
        {
            memory.store_b(phys_addr, reg_x.read());
        }
        else
        {
            reg_x.write(memory.load_b(phys_addr));
        }
    }
};