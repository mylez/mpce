#pragma once
#include "alu.h"
#include "memory.h"
#include "mmio.h"
#include "mmu.h"
#include "register.h"

#include <functional>
#include <queue>

#define OPCODE(inst) (((inst) >> 9) & 0x007f)
#define REG_SEL_X(inst) ((inst)&0x0007)
#define REG_SEL_Y(inst) (((inst) >> 3) & 0x0007)
#define REG_SEL_Z(inst) (((inst) >> 6) & 0x0007)
#define OPCODE_MAP_SIZE (0x80)
#define REGISTER_OP(opcode, op)                                                \
    printf("register opcode %s to op %s\n", #opcode, #op);                     \
    opcode_mapping_.at((opcode) >> 1) = (op)
#define BIND_OP(op) (std::bind(op, this))

namespace MPCE
{

struct CPUState
{
  private:
    using Operation = std::function<void()>;

    std::vector<Operation> opcode_mapping_;

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
    CPUState() : opcode_mapping_{OPCODE_MAP_SIZE, BIND_OP(&CPUState::op_none)}
    {
        // Opcodes appear as multiples of 2 because they are the top 7 bits of
        // the top byte of the instruction word. The lowest bit in the byte is
        // always zero.
        //
        // Syntax quirk: Due to the preprocessor misinterpreting the comma,
        // wrap the macro argument in extra parentheses if it is a function
        // templatized on multiple template parameters.

        // Arithmetic:

        // 00   noop
        // 22   x <- y ^ z
        REGISTER_OP(0x22, BIND_OP((&CPUState::op_alu<0, false, false>)));

        // 24   x <- y - z
        REGISTER_OP(0x24, BIND_OP((&CPUState::op_alu<1, false, false>)));

        // c4   x <- y - z, carry
        REGISTER_OP(0xc4, BIND_OP((&CPUState::op_alu<1, true, false>)));

        // 26   x <- y & z
        REGISTER_OP(0x26, BIND_OP((&CPUState::op_alu<2, false, false>)));

        // 2a   x <- y | z
        REGISTER_OP(0x2a, BIND_OP((&CPUState::op_alu<3, false, false>)));

        // 2c   x <- y + z
        REGISTER_OP(0x2c, BIND_OP((&CPUState::op_alu<4, false, false>)));

        // cc   x <- y + z, carry
        REGISTER_OP(0xcc, BIND_OP((&CPUState::op_alu<4, true, false>)));

        // Arithmetic with immediate:
        // Todo: Is carry really not needed with immediate?

        // 32   x <- y ^ z, imm
        REGISTER_OP(0x32, BIND_OP((&CPUState::op_alu<0, false, true>)));

        // 34   x <- y - z, imm
        REGISTER_OP(0x34, BIND_OP((&CPUState::op_alu<1, false, true>)));

        // 36   x <- y & z, imm
        REGISTER_OP(0x36, BIND_OP((&CPUState::op_alu<2, false, true>)));

        // 3a   x <- y | z, imm
        REGISTER_OP(0x3a, BIND_OP((&CPUState::op_alu<3, false, true>)));

        // 3c   x <- y + z, imm
        REGISTER_OP(0x3a, BIND_OP((&CPUState::op_alu<4, false, true>)));

        // b2   mem_b[y + z] <- x
        REGISTER_OP(0x3a,
                    BIND_OP((&CPUState::op_mem<true, false, true, true>)));

        // b4   mem_b[y + z] <- x, imm
        // b6   x <- mem_bu[y + z]
        // b8   x <- mem_bu[y + z], imm
        // ba   x <- mem_bs[y + z]
        // bc   x <- mem_bs[y + z], imm

        // 42   mem_w[y + z] <- x
        // 44   mem_w[y + z] <- x, imm
        // 46   x <- mem_w[y + z]
        // 48   x <- mem_w[y + z], imm

        // 4a   mem_t[y + z] <- x
        // 4c   mem_t[y + z] <- x, imm
        // 4e   x <- mem_t[y + z]
        // 84   x <- mem_t[y + z], imm
    }

    void cycle()
    {
        load_inst();

        const uint16_t inst_word = inst_.read();
        const Operation operation = opcode_mapping_[OPCODE(inst_word)];
        cout << "opcode is " << (int)inst_word << endl;
        // Perform the operation.
        operation();

        // 1. If interrupt, context switch and branch to isr.
    }

  private:
    bool is_user_mode()
    {
        return status_.read() & 0x8;
    }

    bool is_valid_mode(bool mode)
    {
        return !is_user_mode() || is_user_mode() == mode;
    }

    void load_inst()
    {
        const bool user_mode = is_user_mode();
        const uint16_t pc_addr = register_file_.get(PC).read();
        const uint32_t a_phys_bus =
            mmu_.resolve(pc_addr, ptb_.read(), false, user_mode, false);
        const uint16_t inst_word = mmio_.get_code(user_mode).read(a_phys_bus);

        register_file_.get(PC).write(pc_addr + 1);
        exc_addr_.write(pc_addr);
        inst_.write(inst_word);
    }

    template <uint32_t N> void f()
    {
        cout << "dandy\n";
    }

    /// @tparam alu_sel
    /// @tparam carry_in
    /// @tparam imm
    template <uint32_t alu_sel, bool carry_in, bool imm> void op_alu()
    {
        cout << "performing alu op " << alu_sel << ", carry_in " << carry_in
             << endl;
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
    }

    /// @tparam byte
    /// @tparam mode
    /// @tparam is_data
    /// @tparam write
    template <bool byte, bool mode, bool is_data, bool write> void op_mem()
    {
        cout << "performing mem data load, byte = " << byte << endl;

        if (!is_valid_mode(mode))
        {
            // Todo: Generate interrupt.
            return;
        }

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

        if (mmu_.page_fault() || write && mmu_.read_only_fault())
        {
            // Todo: Generate interrupt and return in case of faults.
        }

        // Todo: make ByteAddressibleMemory and WordAddressibleMemory satisfy a
        // Memory interface, where WordAddressibleMemory ignores the byte
        // parameter. A lot of this control flow can be factored out that way.
        if (is_data)
        {
            const ByteAddressibleMemory &data = mmio_.get_data(mode);

            if (write)
            {
                reg_x.write(data.read(phys_addr, byte));
            }
            else
            {
                data.store(phys_addr, reg_x.read(), byte);
            }
        }
        else
        {
            const WordAddressibleMemory &code = mmio_.get_code(mode);
            if (write)
            {
                reg_x.write(code.read(phys_addr, byte));
            }
            else
            {
                code.store(phys_addr, reg_x.read(), byte);
            }
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
};
} // namespace MPCE
