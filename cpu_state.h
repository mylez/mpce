#pragma once
#include "memory.h"
#include "mmio.h"
#include "mmu.h"
#include "register.h"

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

    std::vector<Operation> opcode_mapping_{OPCODE_MAP_SIZE,
                                           BIND_OP(&CPUState::op_none)};

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

        // Todo: integrate BIND_OP with MAP_OPCODE if all opcodes end up
        // needing BIND_OP anyway.

        // Arithmetic:

        // 00   noop
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

        // Arithmetic with immediate:
        // Todo: is carry really not needed with immediate?

        // 32   x <- y ^ z, imm
        MAP_OPCODE(0x32, BIND_OP((&CPUState::op_alu<0, false, true>)));

        // 34   x <- y - z, imm
        MAP_OPCODE(0x34, BIND_OP((&CPUState::op_alu<1, false, true>)));

        // 36   x <- y & z, imm
        MAP_OPCODE(0x36, BIND_OP((&CPUState::op_alu<2, false, true>)));

        // 3a   x <- y | z, imm
        MAP_OPCODE(0x3a, BIND_OP((&CPUState::op_alu<3, false, true>)));

        // 3c   x <- y + z, imm
        MAP_OPCODE(0x3a, BIND_OP((&CPUState::op_alu<4, false, true>)));

        // Memory:

        // b2   mem_b_kern[y + z] <- x
        // b4   mem_b_kern[y + z] <- x, imm

        // b6   x <- mem_bu_kern[y + z]
        // b8   x <- mem_bu_kern[y + z], imm
        // ba   x <- mem_bs_kern[y + z]
        // bc   x <- mem_bs_kern[y + z], imm

        // 42   mem_w_kern[y + z] <- x
        // 44   mem_w_kern[y + z] <- x, imm
        // 46   x <- mem_w_kern[y + z]
        // 48   x <- mem_w_kern[y+ z], imm

        // 4a   mem_t_kern[y + z] <- x
        // 4c   mem_t_kern[y + z] <- x, imm
        // 4e   x <- mem_t_kern[y + z]
        // 84   x <- mem_t_kern[y + z], imm

        // ??   mem_b_user[y + z] <- x
        // ??   mem_b_user[y + z] <- x, imm

        // ??   x <- mem_bu_user[y + z]
        // ??   x <- mem_bu_user[y + z], imm
        // ??   x <- mem_bs_user[y + z]
        // ??   x <- mem_bs_user[y + z], imm

        // ??   mem_w_user[y + z] <- x
        // ??   mem_w_user[y + z] <- x, imm
        // ??   x <- mem_w_user[y + z]
        // ??   x <- mem_w_user[y+ z], imm

        // ??   mem_t_user[y + z] <- x
        // ??   mem_t_user[y + z] <- x, imm
        // ??   x <- mem_t_user[y + z]
        // ??   x <- mem_t_user[y + z], imm

        // Special registers

        // e0   x <- status
        MAP_OPCODE(0xe0, (op_special_reg_read<true, false, false>(status_)));

        // e2   x <- cause
        MAP_OPCODE(0xe2, (op_special_reg_read<true, false, false>(cause_)));

        // e4   x <- exc_addr
        MAP_OPCODE(0xe2, (op_special_reg_read<true, false, false>(exc_addr_)));

        // e6   x <- eret
        MAP_OPCODE(0xe2, (op_special_reg_read<true, false, false>(eret_)));

        // e8   x <- eret, mode <- !mode
        MAP_OPCODE(0xe2, (op_special_reg_read<true, false, true>(eret_)));

        // ea   x <- y + z, mode <- !mode
        MAP_OPCODE(0xe2, (op_special_reg_read<true, false, true>(eret_)));

        // ec   x <- y + z, imm, mode <- !mode
        MAP_OPCODE(
            0xe2,
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

        // Conditional addition (branching).

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
    }

    void cycle()
    {
        load_inst_word(inst_);

        // Todo: check for interrupts after loading instruction word.

        const uint16_t inst_word = inst_.read();
        const Operation operation = opcode_mapping_[OPCODE(inst_word)];
        cout << "opcode is " << (int)inst_word << endl;

        // Perform the operation.
        operation();

        // Todo: check for interrupts after attempting to complete the
        // operation.
    }

  private:
    void load_inst_word(Register<uint16_t> &reg_x)
    {
        const bool user_mode = is_user_mode();
        const uint16_t pc_addr = register_file_.get(PC).read();
        const uint32_t a_phys_bus =
            mmu_.resolve(pc_addr, ptb_.read(), false, user_mode, false);

        // Todo: Generate interrupt signal and return here for page faults.

        // Todo: Clean up ALL of the printf/cout logging, perhaps using glog.
        cout << "incrementing pc then loading from "
             << (user_mode ? "user" : "kern") << " code to " << reg_x.name()
             << endl;

        const uint16_t word = mmio_.get_code(user_mode).load(a_phys_bus);

        register_file_.get(PC).write(pc_addr + 1);
        exc_addr_.write(pc_addr);
        reg_x.write(word);
    }

    template <bool is_data> void op_store_page_table_entry()
    {
        if (is_user_mode())
        {
            // Todo: Generate interrupt signal and return here.
            return;
        }

        const uint16_t inst_word = inst_.read();

        const uint16_t x = register_file_.get(REG_SEL_X(inst_word)).read();
        const uint16_t y = register_file_.get(REG_SEL_Y(inst_word)).read();
        const uint16_t z = register_file_.get(REG_SEL_Z(inst_word)).read();

        const uint16_t phys_addr = y + z;

        mmu_.page_table(is_data).store(phys_addr, x);
    }

    template <bool protected_inst, bool load_imm, bool toggle_mode,
              typename reg_type_t>
    Operation op_special_reg_read(const Register<reg_type_t> &special_reg)
    {
        return [&]() {
            if (load_imm)
            {
                load_inst_word(register_file_.get(IMM));
            }

            // Todo: If fault, generate interrupt signal and return here.

            if (protected_inst && is_user_mode())
            {
                // Generate illegal instruction interrupt signal and return.
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

    template <bool load_imm, typename reg_type_t>
    Operation op_special_reg_write(Register<reg_type_t> &special_reg)
    {
        return [&]() {
            if (is_user_mode())
            {
                // Todo: generate interrupt signal here and return.
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

    template <typename reg_size_t, Register<reg_size_t> *special_reg,
              bool protected_inst, bool load_imm>
    void op_special_reg_write()
    {
        if (load_imm)
        {
            load_inst_word(register_file_.get(IMM));
        }

        // Todo: If fault, generate interrupts and return here.

        if (protected_inst && is_user_mode())
        {
            // Generate illegal instruction interrupt and return.
        }

        const uint16_t inst_word = inst_.read();
        const uint16_t y = register_file_.get(REG_SEL_Y(inst_word)).read();
        const uint16_t z = register_file_.get(REG_SEL_Z(inst_word)).read();

        special_reg->write(y + z);
    }

    void op_set_mode()
    {
        if (is_user_mode())
        {
            // Generate illegal instruction interrupt and return.
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

        if (is_user_mode() && toggle_mode)
        {
            // Todo: generate illegal inst interrupt and return here.
            return;
        }

        // Load
        if (load_imm)
        {
            load_inst_word(register_file_.get(IMM));
        }

        // Do not proceed with this operation if condition is not satisfied.
        if (static_cast<bool>(cond & status_.read()) != status_invert)
        {
            cout << "condition not satisfied\n";
            return;
        }

        // Todo: generate fault/illegal inst interrupt and return here.

        const uint16_t inst_word = inst_.read();

        Register<uint16_t> &reg_x = register_file_.get(REG_SEL_X(inst_word));
        Register<uint16_t> &reg_y = register_file_.get(REG_SEL_Y(inst_word));
        Register<uint16_t> &reg_z = register_file_.get(REG_SEL_Z(inst_word));

        uint16_t x = 0;
        uint16_t y = reg_y.read();
        uint16_t z = reg_z.read();

        bool update_status = false;
        bool update_status_unsigned = false;
        bool carry_out = false;
        bool overflow = false;
        bool zero = false;
        bool negative = false;

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

        reg_z.write(z);

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
    /// @tparam store
    /// @tparam load_imm
    template <bool byte, bool mode, bool is_data, bool store, bool load_imm>
    void op_mem()
    {
        cout << "performing mem data load, byte = " << byte << endl;

        if (!is_valid_mode(mode))
        {
            // Todo: Generate interrupt.
            return;
        }

        if (load_imm)
        {
            load_inst_word(register_file_.get(IMM));
        }

        // Todo: generate page fault interrupts / return here.

        const uint16_t inst_word = inst_.read();

        Register<uint16_t> &reg_x = register_file_.get(REG_SEL_X(inst_word));

        const uint16_t y = register_file_.get(REG_SEL_Y(inst_word)).read();
        const uint16_t z = register_file_.get(REG_SEL_Z(inst_word)).read();

        mmu_.reset_fault();

        const uint16_t virt_addr = y + z;
        const uint32_t phys_addr =
            is_user_mode()
                ? mmu_.resolve(virt_addr, ptb_.read(), true, true, false)
                : virt_addr;

        if (mmu_.page_fault() || store && mmu_.read_only_fault())
        {
            // Todo: Generate interrupt and return in case of faults.
        }

        Memory &memory = is_data ? mmio_.get_data(mode) : mmio_.get_code(mode);

        if (store)
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
        // Todo: Generate interrupt.
    }

    void op_none()
    {
    }

    bool is_user_mode() const
    {
        return status_.read() & 0x8;
    }

    bool is_valid_mode(bool mode) const
    {
        const bool is_user = is_user_mode();
        return !is_user || is_user && mode;
    }
};
} // namespace MPCE
