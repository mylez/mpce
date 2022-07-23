#pragma once
#include "alu.h"
#include "memory.h"
#include "mmio.h"
#include "mmu.h"
#include "register.h"

#include <queue>

#define OPCODE(inst) (((inst) >> 9) & 0x007f)
#define REG_SEL_X(inst) ((inst)&0x0007)
#define REG_SEL_Y(inst) (((inst) >> 3) & 0x0007)
#define REG_SEL_Z(inst) (((inst) >> 6) & 0x0007)
#define OPCODE_MAP_SIZE (0x80)
#define REGISTER_OP(opcode, op)                                                \
    printf("register opcode %s to op %s\n", #opcode, #op);                     \
    opcode_mapping_.at((opcode) >> 1) = (op)

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
    CPUState() : opcode_mapping_{OPCODE_MAP_SIZE, op_alu(0, false)}
    {
        // Todo: define all these.
        REGISTER_OP(0x24, op_alu(1, false));
        REGISTER_OP(0xc4, op_alu(2, false));
        REGISTER_OP(0x26, op_alu(3, false));
        REGISTER_OP(0x2a, op_alu(4, false));
        REGISTER_OP(0x2c, op_alu(5, false));
        REGISTER_OP(0xcc, op_alu(6, false));

        REGISTER_OP(0x32, op_alu(0, false));
        REGISTER_OP(0x34, op_alu(0, false));
        REGISTER_OP(0x36, op_alu(0, false));
        REGISTER_OP(0x3a, op_alu(0, false));
        REGISTER_OP(0x3c, op_alu(0, false));

        REGISTER_OP(0xb2, op_alu(0, false));
        REGISTER_OP(0xb4, op_alu(0, false));
        REGISTER_OP(0xb4, op_alu(0, false));
        REGISTER_OP(0xb6, op_alu(0, false));
        REGISTER_OP(0xb8, op_alu(0, false));
        REGISTER_OP(0xba, op_alu(0, false));
        REGISTER_OP(0xbc, op_alu(0, false));

        REGISTER_OP(0x42, op_alu(0, false));
        REGISTER_OP(0x44, op_alu(0, false));
        REGISTER_OP(0x46, op_alu(0, false));
        REGISTER_OP(0x48, op_alu(0, false));

        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4c, op_alu(0, false));
        REGISTER_OP(0x4e, op_alu(0, false));
        REGISTER_OP(0x84, op_alu(0, false));

        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4a, op_alu(0, false));

        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4a, op_alu(0, false));

        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4a, op_alu(0, false));
        REGISTER_OP(0x4a, op_alu(0, false));
    }

    void cycle()
    {
        load_inst();

        const uint16_t inst_word = inst_.read();
        const Operation operation = opcode_mapping_[OPCODE(inst_word)];

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

    Operation op_alu(uint32_t alu_sel, bool carry_in)
    {
        return [&, alu_sel, carry_in]() {
            cout << "performing alu op " << alu_sel << ", carry_in " << carry_in
                 << endl;
            const uint16_t inst_word = inst_.read();

            Register<uint16_t> &reg_x =
                register_file_.get(REG_SEL_X(inst_word));
            Register<uint16_t> &reg_y =
                register_file_.get(REG_SEL_Y(inst_word));
            Register<uint16_t> &reg_z =
                register_file_.get(REG_SEL_Z(inst_word));

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
        };
    }

    Operation op_mem_data_load(bool byte, bool mode)
    {
        return [&, byte, mode]() {
            cout << "performing mem data load, byte = " << byte << endl;

            if (!is_valid_mode(mode))
            {
                // Todo: Generate interrupt.
                return;
            }

            const uint16_t inst_word = inst_.read();

            Register<uint16_t> &reg_x =
                register_file_.get(REG_SEL_X(inst_word));

            const uint16_t y = register_file_.get(REG_SEL_Y(inst_word)).read();
            const uint16_t z = register_file_.get(REG_SEL_Z(inst_word)).read();
            const uint16_t virt_addr = y + z;

            mmu_.reset_fault();

            const uint32_t phys_addr =
                is_user_mode()
                    ? mmu_.resolve(virt_addr, ptb_.read(), true, true, false)
                    : virt_addr;

            if (mmu_.page_fault())
            {
                // Todo: Generate interrupt and return in case of faults.
            }

            const ByteAddressibleMemory &data = mmio_.get_data(mode);
            reg_x.write(data.read(phys_addr, byte));
        };
    }

    Operation op_mem_data_store(bool byte, bool mode)
    {
        return [&, byte, mode]() {
            cout << "performing mem data store, byte = " << byte << endl;

            if (!is_valid_mode(mode))
            {
                // Todo: Generate interrupt.
                return;
            }

            const uint16_t inst_word = inst_.read();

            const uint16_t x = register_file_.get(REG_SEL_X(inst_word)).read();
            const uint16_t y = register_file_.get(REG_SEL_Y(inst_word)).read();
            const uint16_t z = register_file_.get(REG_SEL_Z(inst_word)).read();
            const uint16_t virt_addr = y + z;

            mmu_.reset_fault();

            const uint32_t phys_addr =
                is_user_mode()
                    ? mmu_.resolve(virt_addr, ptb_.read(), true, true, true)
                    : virt_addr;

            if (mmu_.page_fault())
            {
                // Todo: Generate interrupt and return in case of faults.
            }
            else if (mmu_.read_only_fault())
            {
                // Todo: Generate interrupt and return in case of faults.
            }

            ByteAddressibleMemory &data = mmio_.get_data(mode);

            if (byte)
            {
                data.store_byte(phys_addr, static_cast<uint8_t>(x));
            }
            else
            {
                data.store_word(phys_addr, x);
            }
        };
    }

    void op_invalid()
    {
        cerr << "invalid operation." << endl;
        // Todo: Generate interrupt.
    }
};
} // namespace MPCE