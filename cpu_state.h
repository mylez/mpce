#pragma once
#include "alu.h"
#include "memory.h"
#include "mmio.h"
#include "mmu.h"
#include "register.h"

#include <queue>

#define BIND_OPERATION(op) (std::bind(&CPUState::op, this))
#define OPCODE(inst) (((inst) >> 7) & 0x01ff)
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

    std::queue<Operation> next_ops_;

    std::vector<Operation> opcode_mapping_;

    /// @brief Main architectural registers, including program
    /// counter.
    RegisterFile register_file_;

    /// @brief Memory management unit, maps virtual address to
    /// physical address in user mode.
    MMU mmu_;

    MMIO mmio_;

    /// @brief Special registers.
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
    CPUState() : opcode_mapping_{OPCODE_MAP_SIZE, BIND_OPERATION(op_none)}
    {
        // Todo: define all these.
        REGISTER_OP(0x00, BIND_OPERATION(op_none));

        REGISTER_OP(0x22, op_alu(0, false));
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
        next_ops_.emplace(BIND_OPERATION(op_load));
        next_ops_.emplace(op_alu(0x00, false));

        // Example setting an opcode mapping.
        // Note: Opcode and register arguments overlap!
        opcode_mapping_.at(0x0ab) = op_alu(0x00, false);

        for (int cycle = 0; !next_ops_.empty(); cycle++)
        {
            printf(" - - - - - - cycle %d - - - - - -\n", cycle);
            next_ops_.front()();
            next_ops_.pop();
        }
    }

  private:
    bool is_user_mode()
    {
        return status_.read() & 0x8;
    }

    void op_load()
    {
        const bool user_mode = is_user_mode();
        const uint16_t pc_addr = register_file_.get(PC).read();
        const uint32_t a_phys_bus =
            mmu_.resolve(pc_addr, ptb_.read(), false, user_mode, false);
        const uint16_t inst_word = mmio_.get_code(user_mode).read(a_phys_bus);

        register_file_.get(PC).write(pc_addr + 1);
        exc_addr_.write(pc_addr);
        inst_.write(inst_word);

        const uint16_t opcode = OPCODE(inst_word);
    }

    Operation op_alu(uint32_t alu_sel, bool carry_in)
    {
        return [=]() {
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

    void op_none()
    {
    }
};
} // namespace MPCE