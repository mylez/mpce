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
#define BIND_OP(op) (std::bind(op, this))
#define MAP_OPCODE(opcode, op)                                                 \
    printf("mapping opcode %s: %s\n", #opcode, #op);                           \
    opcode_mapping_.at((opcode) >> 1) = (op)

#define STATUS_NEGATIVE 0x08
#define STATUS_ZERO 0x04
#define STATUS_CARRY 0x02
#define STATUS_OVERFLOW 0x02

#define OPCODE_MAP_SIZE 0x80

namespace MPCE
{

struct CPUState
{
  private:
    using Operation = std::function<void()>;

    /// Map a 7-bit opcode to one of 128 Operation functions. Initialize entries
    /// to op_invalid.
    std::vector<Operation> opcode_mapping_{OPCODE_MAP_SIZE,
                                           BIND_OP(&CPUState::op_invalid)};

    /// Main architectural registers, including program counter.
    RegisterFile register_file_;

    /// Memory management unit, maps virtual address to physical address in user
    /// mode.
    MMU mmu_;

    /// Memory-mapped IO.
    MMIO mmio_;

    /// Special registers.
    Register<uint8_t> status_{"status", 0xf0};
    Register<uint8_t> cause_{"cause"};
    Register<uint16_t> eret_{"eret"};
    Register<uint16_t> context_{"context"};
    Register<uint16_t> timer_{"timer"};
    Register<uint16_t> isr_{"isr"};
    Register<uint16_t> ptb_{"ptb"};
    Register<uint16_t> exc_addr_{"exc_addr"};
    Register<uint16_t> inst_{"inst"};
    Register<uint8_t> mode_{"mode", 0xfe};

    Interrupt interrupt_;

  public:
    CPUState()
    {
        // Opcodes appear as multiples of 2 because they are the top 7 bits of
        // the top byte of the instruction word. The lowest bit in the byte is
        // always zero.
        //
        // Syntax quirk: Due to the preprocessor misinterpreting the comma,
        // wrap the macro argument in extra parentheses if it is a function
        // templatized on multiple template parameters.

        // 00   noop
        MAP_OPCODE(0x00, BIND_OP(&CPUState::op_none));

        // Arithmetic:

        // 22   x <- y ^ z
        MAP_OPCODE(0x22, BIND_OP((&CPUState::op_alu<0, false, false>)));

        // 24   x <- y - z
        MAP_OPCODE(0x24, BIND_OP((&CPUState::op_alu<1, false, false>)));

        // c4   x <- y - z, carry
        MAP_OPCODE(0xc4, BIND_OP((&CPUState::op_alu<1, true, false>)));

        // 26   x <- y & z
        MAP_OPCODE(0x26, BIND_OP((&CPUState::op_alu<2, false, false>)));

        // 2a   x <- y | z
        MAP_OPCODE(0x2a, BIND_OP((&CPUState::op_alu<3, false, false>)));

        // 2c   x <- y + z
        MAP_OPCODE(0x2c, BIND_OP((&CPUState::op_alu<4, false, false>)));

        // cc   x <- y + z, carry
        MAP_OPCODE(0xcc, BIND_OP((&CPUState::op_alu<4, true, false>)));

        // 32   x <- y ^ z, imm
        MAP_OPCODE(0x32, BIND_OP((&CPUState::op_alu<0, false, true>)));

        // 34   x <- y - z, imm
        MAP_OPCODE(0x34, BIND_OP((&CPUState::op_alu<1, false, true>)));

        // 36   x <- y & z, imm
        MAP_OPCODE(0x36, BIND_OP((&CPUState::op_alu<2, false, true>)));

        // 3a   x <- y | z, imm
        MAP_OPCODE(0x3a, BIND_OP((&CPUState::op_alu<3, false, true>)));

        // 3c   x <- y + z, imm
        MAP_OPCODE(0x3c, BIND_OP((&CPUState::op_alu<4, false, true>)));

        // Memory and IO:

        // b2   mem_b_kern[y + z] <- x
        MAP_OPCODE(
            0xb2,
            BIND_OP((&CPUState::op_mem<true, false, true, true, false, true>)));

        // b4   mem_b_kern[y + z] <- x, imm
        MAP_OPCODE(
            0xb4,
            BIND_OP((&CPUState::op_mem<true, false, true, true, true, true>)));

        // b6   x <- mem_bu_kern[y + z]
        MAP_OPCODE(
            0xb6,
            BIND_OP(
                (&CPUState::op_mem<true, false, true, true, false, false>)));

        // b8   x <- mem_bu_kern[y + z], imm
        MAP_OPCODE(
            0xb8,
            BIND_OP((&CPUState::op_mem<true, false, true, true, true, false>)));

        // ba   x <- mem_bs_kern[y + z]
        MAP_OPCODE(
            0xba,
            BIND_OP(
                (&CPUState::op_mem<true, false, true, false, false, true>)));

        // bc   x <- mem_bs_kern[y + z], imm
        MAP_OPCODE(
            0xbc,
            BIND_OP((&CPUState::op_mem<true, false, true, false, true, true>)));

        // 42   mem_w_kern[y + z] <- x
        MAP_OPCODE(
            0x42,
            BIND_OP(
                (&CPUState::op_mem<false, false, true, true, false, false>)));

        // 44   mem_w_kern[y + z] <- x, imm
        MAP_OPCODE(
            0x44,
            BIND_OP(
                (&CPUState::op_mem<false, false, true, true, true, false>)));

        // 46   x <- mem_w_kern[y + z]
        MAP_OPCODE(
            0x46,
            BIND_OP(
                (&CPUState::op_mem<false, false, true, false, false, false>)));

        // 48   x <- mem_w_kern[y+ z], imm
        MAP_OPCODE(
            0x48,
            BIND_OP(
                (&CPUState::op_mem<false, false, true, false, true, false>)));

        // 4a   mem_t_kern[y + z] <- x
        MAP_OPCODE(
            0x4a,
            BIND_OP(
                (&CPUState::op_mem<false, false, false, true, false, false>)));

        // 4c   mem_t_kern[y + z] <- x, imm
        MAP_OPCODE(
            0x4c,
            BIND_OP(
                (&CPUState::op_mem<false, false, false, true, true, false>)));

        // 4e   x <- mem_t_kern[y + z]
        MAP_OPCODE(
            0x4e,
            BIND_OP(
                (&CPUState::op_mem<false, false, false, false, false, false>)));

        // 6e   x <- mem_t_kern[y + z], imm
        MAP_OPCODE(
            0x6e,
            BIND_OP(
                (&CPUState::op_mem<false, false, false, false, true, false>)));

        // 72   mem_b_user[y + z] <- x
        MAP_OPCODE(
            0x72,
            BIND_OP((&CPUState::op_mem<true, true, true, true, false, true>)));

        // 74   mem_b_user[y + z] <- x, imm
        MAP_OPCODE(
            0x74,
            BIND_OP((&CPUState::op_mem<true, true, true, true, true, true>)));

        // 76   x <- mem_bu_user[y + z]
        MAP_OPCODE(
            0x76,
            BIND_OP((&CPUState::op_mem<true, true, true, true, true, false>)));

        // 78   x <- mem_bu_user[y + z], imm
        MAP_OPCODE(
            0x78,
            BIND_OP((&CPUState::op_mem<true, true, true, false, true, false>)));

        // 7a   x <- mem_bs_user[y + z]
        MAP_OPCODE(
            0x7a,
            BIND_OP((&CPUState::op_mem<true, true, true, false, false, true>)));

        // 7c   x <- mem_bs_user[y + z], imm
        MAP_OPCODE(
            0x7c,
            BIND_OP((&CPUState::op_mem<true, true, true, false, true, true>)));

        // 7e   mem_w_user[y + z] <- x
        MAP_OPCODE(
            0x7e,
            BIND_OP(
                (&CPUState::op_mem<false, true, true, true, false, false>)));

        // 82   mem_w_user[y + z] <- x, imm
        MAP_OPCODE(
            0x82,
            BIND_OP((&CPUState::op_mem<false, true, true, true, true, false>)));

        // 84   x <- mem_w_user[y + z]
        MAP_OPCODE(
            0x84,
            BIND_OP(
                (&CPUState::op_mem<false, true, true, false, false, false>)));

        // 86   x <- mem_w_user[y + z], imm
        MAP_OPCODE(
            0x86,
            BIND_OP(
                (&CPUState::op_mem<false, true, true, false, true, false>)));

        // 88   mem_t_user[y + z] <- x
        MAP_OPCODE(
            0x88,
            BIND_OP(
                (&CPUState::op_mem<false, true, false, true, false, false>)));

        // 8a   mem_t_user[y + z] <- x, imm
        MAP_OPCODE(
            0x8a,
            BIND_OP(
                (&CPUState::op_mem<false, true, false, true, true, false>)));

        // 8c   x <- mem_t_user[y + z]
        MAP_OPCODE(
            0x8c,
            BIND_OP(
                (&CPUState::op_mem<false, true, false, false, false, false>)));

        // 8e   x <- mem_t_user[y + z], imm
        MAP_OPCODE(
            0x8e,
            BIND_OP(
                (&CPUState::op_mem<false, true, false, false, true, false>)));

        // Special registers:

        // e0   x <- status
        MAP_OPCODE(0xe0, (op_special_reg_read<true, false, false>(status_)));

        // e2   x <- cause
        MAP_OPCODE(0xe2, (op_special_reg_read<true, false, false>(cause_)));

        // e4   x <- exc_addr
        MAP_OPCODE(0xe4, (op_special_reg_read<true, false, false>(exc_addr_)));

        // e6   x <- eret
        MAP_OPCODE(0xe6, (op_special_reg_read<true, false, false>(eret_)));

        // e8   x <- eret, mode <- !mode
        MAP_OPCODE(0xe8, (op_special_reg_read<true, false, true>(eret_)));

        // ea   x <- y + z, mode <- !mode
        MAP_OPCODE(0xea, (op_special_reg_read<true, false, true>(eret_)));

        // ec   x <- y + z, imm, mode <- !mode
        MAP_OPCODE(
            0xec,
            BIND_OP((&CPUState::op_alu<4, false, true, 0xff, false, true>)));

        // f0   mode <- 1
        MAP_OPCODE(0xf0, BIND_OP(&CPUState::op_set_mode));

        // f2   ptb <- y + z
        MAP_OPCODE(0xf2, op_special_reg_write<false>(ptb_));

        // f4   timer <- y + z, imm
        MAP_OPCODE(0xf4, op_special_reg_write<true>(timer_));

        // f6   isr <- y + z
        MAP_OPCODE(0xf6, op_special_reg_write<false>(isr_));

        // f8   status <- y + z
        MAP_OPCODE(0xf8, op_special_reg_write<false>(status_));

        // fa   mmu_t[y + z] <- x
        MAP_OPCODE(0xfa, BIND_OP(&CPUState::op_store_page_table_entry<false>));

        // fc   mmu_d[y + z] <- x
        MAP_OPCODE(0xfc, BIND_OP(&CPUState::op_store_page_table_entry<true>));

        // Conditional branching:

        // 20   x <- y + z if zero
        MAP_OPCODE(0x20,
                   BIND_OP((&CPUState::op_alu<4, false, false, STATUS_ZERO>)));

        // 30   x <- y + z if zero, imm
        MAP_OPCODE(0x30,
                   BIND_OP((&CPUState::op_alu<4, false, true, STATUS_ZERO>)));

        // 40   x <- y + z if not zero
        MAP_OPCODE(
            0x40,
            BIND_OP((&CPUState::op_alu<4, false, false, STATUS_ZERO, true>)));

        // 50   x <- y + z if not zero, imm
        MAP_OPCODE(
            0x50,
            BIND_OP((&CPUState::op_alu<4, false, true, STATUS_ZERO, true>)));

        // 60   x <- y + z if neg
        MAP_OPCODE(
            0x60,
            BIND_OP((&CPUState::op_alu<4, false, false, STATUS_NEGATIVE>)));

        // 70   x <- y + z if neg, imm
        MAP_OPCODE(
            0x70,
            BIND_OP((&CPUState::op_alu<4, false, true, STATUS_NEGATIVE>)));

        // 80   x <- y + z if pos
        MAP_OPCODE(
            0x80,
            BIND_OP((&CPUState::op_alu<4, false, false,
                                       STATUS_NEGATIVE | STATUS_ZERO, true>)));

        // 90   x <- y + z if pos, imm
        MAP_OPCODE(
            0x90,
            BIND_OP((&CPUState::op_alu<4, false, true,
                                       STATUS_NEGATIVE | STATUS_ZERO, true>)));

        // a0   x <- y + z if carry
        MAP_OPCODE(0xa0,
                   BIND_OP((&CPUState::op_alu<4, false, false, STATUS_CARRY>)));

        // b0   x <- y + z if carry, imm
        MAP_OPCODE(0xb0,
                   BIND_OP((&CPUState::op_alu<4, false, true, STATUS_CARRY>)));

        // c0   x <- y + z if overflow
        MAP_OPCODE(
            0xc0,
            BIND_OP((&CPUState::op_alu<4, false, false, STATUS_OVERFLOW>)));

        // d0   x <- y + z if overflow, imm
        MAP_OPCODE(
            0xd0,
            BIND_OP((&CPUState::op_alu<4, false, true, STATUS_OVERFLOW>)));

        // Todo: Adding the following instructions:
        //   1) Load immediate into any destination register (user and kern).
        //   2) Atomic test and set (user and kern).
        //   3) Function call (branch and link).
        //   4) System call (software trap/interrupt, return value in r7).
        //   5) Loading page table entries (code and data) into the register
        //   file.
        //   6) The ability to execute code (including protected instructions)
        //   from a virtual page while in kern mode (branch and toggle virtual
        //   memory).
    }

    void cycle()
    {
        static int cycle_num = 0;
        printf("\ncycle %d:\n", ++cycle_num);
        printf("mode=%s\n", mode_.read() ? "USER" : "KERN");

        const bool cycle_began_as_user = is_user_mode();

        if (cycle_began_as_user)
        {
            context_switch_to_isr_if({TIME_OUT, IRQ0, IRQ1, IRQ2, IRQ3});
        }

        load_inst_word(inst_);

        if (cycle_began_as_user)
        {
            context_switch_to_isr_if({PG_FAULT});
            return;
        }

        // Note: Interrupts are not checked in kernel mode. Kernel mode cannot
        // handle interrupts recursively. If an interrupt occurs in kernel mode,
        // the the system will either crash, or enter an undefined state, or the
        // interrupt may simply be ignored (as with read-only faults in kernel
        // mode).

        // Clear previous interrupt signal state. Todo: allow for external
        // interrupt requests via IRQN, which are not be cleared internally.
        interrupt_.clear();

        // Save a potential exception return address in user mode.
        if (cycle_began_as_user)
        {
            eret_.write(register_file_.get(PC).read());
        }

        const uint16_t inst_word = inst_.read();
        const Operation inst_op = opcode_mapping_[OPCODE(inst_word)];

        // Perform the operation.
        inst_op();

        if (cycle_began_as_user)
        {
            context_switch_to_isr_if({PG_FAULT, RO_FAULT, ILL_INST});
            return;
        }
    }

  private:
    void context_switch_to_isr_if(const std::vector<InterruptSignal> signals)
    {
        if (!interrupt_.is_signalled(signals))
        {
            return;
        }

        cause_.write(interrupt_.cause());

        // Save a general purpose register in the context special register,
        // so that the general purpose register is free to hold an address
        // value. This is needed for storing the rest of the cpu context
        // during an isr.
        context_.write(register_file_.get(IMM).read());

        // Branch to the interrupt service routine (isr) and switch to
        // kernel mode.
        register_file_.get(PC).write(isr_.read());
        mode_.write(0);
    }

    /// @param reg_x
    void load_inst_word(Register<uint16_t> &reg_x)
    {
        const bool user_mode = is_user_mode();
        const uint16_t pc_addr = register_file_.get(PC).read();
        const uint32_t a_phys_bus =
            user_mode
                ? mmu_.resolve(pc_addr, ptb_.read(), false, false, interrupt_)
                : pc_addr;

        if (interrupt_.is_signalled({PG_FAULT}))
        {
            return;
        }

        // Todo: Clean up ALL of the printf/cout logging, perhaps using
        // glog.
        cout << "incrementing pc then loading from "
             << (user_mode ? "user" : "kern") << " code to " << reg_x.name()
             << endl;

        const uint16_t word = mmio_.get_code(user_mode).load(a_phys_bus);

        register_file_.get(PC).write(pc_addr + 1);

        if (user_mode)
        {
            exc_addr_.write(pc_addr);
        }

        reg_x.write(word);
    }

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

        mmu_.page_table(is_data).store(phys_addr, x);
    }

    /// @tparam protected_inst
    /// @tparam load_imm
    /// @tparam toggle_mode
    /// @tparam reg_type_t
    /// @param special_reg
    /// @returns An Operation object that can be called like a function.
    template <bool protected_inst, bool load_imm, bool toggle_mode,
              typename reg_type_t>
    Operation op_special_reg_read(const Register<reg_type_t> &special_reg)
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

            Register<uint16_t> &reg_x =
                register_file_.get(REG_SEL_X(inst_word));

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
    Operation op_special_reg_write(Register<reg_type_t> &special_reg)
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
            }

            // Todo: Check mmu faults, generate interrupts and return if needed.

            const uint16_t inst_word = inst_.read();
            const uint16_t y = register_file_.get(REG_SEL_Y(inst_word)).read();
            const uint16_t z = register_file_.get(REG_SEL_Z(inst_word)).read();

            const uint16_t value = y + z;

            special_reg.write(value);
        };
    }

    /// Enable user mode by setting the mode register to 1.
    void op_set_mode()
    {
        if (is_user_mode())
        {
            interrupt_.signal(ILL_INST);
            return;
        }

        mode_.write(1);
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
        cout << "performing alu op " << alu_sel << ", carry_in " << carry_in
             << endl;

        const bool user_mode = is_user_mode();

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
            cout << "condition not satisfied\n";
            return;
        }

        const uint16_t inst_word = inst_.read();

        const uint16_t y = register_file_.get(REG_SEL_Y(inst_word)).read();
        const uint16_t z = register_file_.get(REG_SEL_Z(inst_word)).read();

        Register<uint16_t> &reg_x = register_file_.get(REG_SEL_X(inst_word));

        uint16_t x = 0;

        bool update_status = false;
        bool update_status_unsigned = false;
        bool carry_out = false;
        bool overflow = false;
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

        cout << "mem data load " << byte << " " << inst_mode << " " << is_data
             << " " << is_store << " " << load_imm << " " << sign_extend_byte
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

        Register<uint16_t> &reg_x = register_file_.get(REG_SEL_X(inst_word));

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

        Memory &memory =
            is_data ? mmio_.get_data(inst_mode) : mmio_.get_code(inst_mode);

        if (is_store)
        {
            memory.store(phys_addr, reg_x.read(), byte);
        }
        else
        {
            reg_x.write(memory.load(phys_addr, byte));
        }
    }

    void op_invalid()
    {
        cerr << "invalid operation." << endl;
        interrupt_.signal(ILL_INST);
    }

    void op_none()
    {
        cout << " * * * * op_none * * * *\n";
    }

    bool is_user_mode() const
    {
        return status_.read() & 0x8;
    }
};
} // namespace MPCE
