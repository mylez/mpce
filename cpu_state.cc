#include "cpu_state.h"

using namespace std;

cpu_state_t::cpu_state_t()
{
    // Opcodes appear as multiples of 2 because they are the top 7 bits of
    // the top byte of the instruction word. The lowest bit in the byte is
    // always zero.
    //
    // Syntax quirk: Due to the preprocessor misinterpreting the comma,
    // wrap the macro argument in extra parentheses if it is a function
    // templated on multiple template parameters.

    // 00   noop
    MAP_OPCODE(0x00, BIND_OP(&cpu_state_t::op_none));

    // Arithmetic:

    // 22   x <- y ^ z
    MAP_OPCODE(0x22, BIND_OP((&cpu_state_t::op_alu<0, false, false>)));

    // 24   x <- y - z
    MAP_OPCODE(0x24, BIND_OP((&cpu_state_t::op_alu<1, false, false>)));

    // c4   x <- y - z, carry
    MAP_OPCODE(0xc4, BIND_OP((&cpu_state_t::op_alu<1, true, false>)));

    // 26   x <- y & z
    MAP_OPCODE(0x26, BIND_OP((&cpu_state_t::op_alu<2, false, false>)));

    // 2a   x <- y | z
    MAP_OPCODE(0x2a, BIND_OP((&cpu_state_t::op_alu<3, false, false>)));

    // 2c   x <- y + z
    MAP_OPCODE(0x2c, BIND_OP((&cpu_state_t::op_alu<4, false, false>)));

    // cc   x <- y + z, carry
    MAP_OPCODE(0xcc, BIND_OP((&cpu_state_t::op_alu<4, true, false>)));

    // 32   x <- y ^ z, imm
    MAP_OPCODE(0x32, BIND_OP((&cpu_state_t::op_alu<0, false, true>)));

    // 34   x <- y - z, imm
    MAP_OPCODE(0x34, BIND_OP((&cpu_state_t::op_alu<1, false, true>)));

    // 36   x <- y & z, imm
    MAP_OPCODE(0x36, BIND_OP((&cpu_state_t::op_alu<2, false, true>)));

    // 3a   x <- y | z, imm
    MAP_OPCODE(0x3a, BIND_OP((&cpu_state_t::op_alu<3, false, true>)));

    // 3c   x <- y + z, imm
    MAP_OPCODE(0x3c, BIND_OP((&cpu_state_t::op_alu<4, false, true>)));

    // memory and IO:

    // b2   mem_b_kern[y + z] <- x
    MAP_OPCODE(
        0xb2,
        BIND_OP((&cpu_state_t::op_mem<true, false, true, true, false, true>)));

    // b4   mem_b_kern[y + z] <- x, imm
    MAP_OPCODE(
        0xb4,
        BIND_OP((&cpu_state_t::op_mem<true, false, true, true, true, true>)));

    // b6   x <- mem_bu_kern[y + z]
    MAP_OPCODE(
        0xb6,
        BIND_OP((&cpu_state_t::op_mem<true, false, true, true, false, false>)));

    // b8   x <- mem_bu_kern[y + z], imm
    MAP_OPCODE(
        0xb8,
        BIND_OP((&cpu_state_t::op_mem<true, false, true, false, true, false>)));

    // ba   x <- mem_bs_kern[y + z]
    MAP_OPCODE(
        0xba,
        BIND_OP((&cpu_state_t::op_mem<true, false, true, false, false, true>)));

    // bc   x <- mem_bs_kern[y + z], imm
    MAP_OPCODE(
        0xbc,
        BIND_OP((&cpu_state_t::op_mem<true, false, true, false, true, true>)));

    // 42   mem_w_kern[y + z] <- x
    MAP_OPCODE(
        0x42,
        BIND_OP(
            (&cpu_state_t::op_mem<false, false, true, true, false, false>)));

    // 44   mem_w_kern[y + z] <- x, imm
    MAP_OPCODE(
        0x44,
        BIND_OP((&cpu_state_t::op_mem<false, false, true, true, true, false>)));

    // 46   x <- mem_w_kern[y + z]
    MAP_OPCODE(
        0x46,
        BIND_OP(
            (&cpu_state_t::op_mem<false, false, true, false, false, false>)));

    // 48   x <- mem_w_kern[y+ z], imm
    MAP_OPCODE(
        0x48,
        BIND_OP(
            (&cpu_state_t::op_mem<false, false, true, false, true, false>)));

    // 4a   mem_t_kern[y + z] <- x
    MAP_OPCODE(
        0x4a,
        BIND_OP(
            (&cpu_state_t::op_mem<false, false, false, true, false, false>)));

    // 4c   mem_t_kern[y + z] <- x, imm
    MAP_OPCODE(
        0x4c,
        BIND_OP(
            (&cpu_state_t::op_mem<false, false, false, true, true, false>)));

    // 4e   x <- mem_t_kern[y + z]
    MAP_OPCODE(
        0x4e,
        BIND_OP(
            (&cpu_state_t::op_mem<false, false, false, false, false, false>)));

    // 6c   x <- mem_bs_user[y + z], mem_bs_user[y + z] <- imm
    MAP_OPCODE(0x6c, BIND_OP((&cpu_state_t::op_ats)));

    // 6e   x <- mem_t_kern[y + z], imm
    MAP_OPCODE(
        0x6e,
        BIND_OP(
            (&cpu_state_t::op_mem<false, false, false, false, true, false>)));

    // 72   mem_b_user[y + z] <- x
    MAP_OPCODE(
        0x72,
        BIND_OP((&cpu_state_t::op_mem<true, true, true, true, false, true>)));

    // 74   mem_b_user[y + z] <- x, imm
    MAP_OPCODE(
        0x74,
        BIND_OP((&cpu_state_t::op_mem<true, true, true, true, true, true>)));

    // 76   x <- mem_bu_user[y + z]
    MAP_OPCODE(
        0x76,
        BIND_OP((&cpu_state_t::op_mem<true, true, true, true, true, false>)));

    // 78   x <- mem_bu_user[y + z], imm
    MAP_OPCODE(
        0x78,
        BIND_OP((&cpu_state_t::op_mem<true, true, true, false, true, false>)));

    // 7a   x <- mem_bs_user[y + z]
    MAP_OPCODE(
        0x7a,
        BIND_OP((&cpu_state_t::op_mem<true, true, true, false, false, true>)));

    // 7c   x <- mem_bs_user[y + z], imm
    MAP_OPCODE(
        0x7c,
        BIND_OP((&cpu_state_t::op_mem<true, true, true, false, true, true>)));

    // 7e   mem_w_user[y + z] <- x
    MAP_OPCODE(
        0x7e,
        BIND_OP((&cpu_state_t::op_mem<false, true, true, true, false, false>)));

    // 82   mem_w_user[y + z] <- x, imm
    MAP_OPCODE(
        0x82,
        BIND_OP((&cpu_state_t::op_mem<false, true, true, true, true, false>)));

    // 84   x <- mem_w_user[y + z]
    MAP_OPCODE(
        0x84,
        BIND_OP(
            (&cpu_state_t::op_mem<false, true, true, false, false, false>)));

    // 86   x <- mem_w_user[y + z], imm
    MAP_OPCODE(
        0x86,
        BIND_OP((&cpu_state_t::op_mem<false, true, true, false, true, false>)));

    // 88   mem_t_user[y + z] <- x
    MAP_OPCODE(
        0x88,
        BIND_OP(
            (&cpu_state_t::op_mem<false, true, false, true, false, false>)));

    // 8a   mem_t_user[y + z] <- x, imm
    MAP_OPCODE(
        0x8a,
        BIND_OP((&cpu_state_t::op_mem<false, true, false, true, true, false>)));

    // 8c   x <- mem_t_user[y + z]
    MAP_OPCODE(
        0x8c,
        BIND_OP(
            (&cpu_state_t::op_mem<false, true, false, false, false, false>)));

    // 8e   x <- mem_t_user[y + z], imm
    MAP_OPCODE(
        0x8e,
        BIND_OP(
            (&cpu_state_t::op_mem<false, true, false, false, true, false>)));

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
        BIND_OP((&cpu_state_t::op_alu<4, false, true, 0xff, false, true>)));

    // f0   mode <- 1
    MAP_OPCODE(0xf0, BIND_OP(&cpu_state_t::op_set_mode));

    // f2   ptb <- y + z
    MAP_OPCODE(0xf2, op_special_reg_write<false>(ptb_));

    // f4   timer <- y + z, imm
    MAP_OPCODE(0xf4, op_special_reg_write<true>(timer_));

    // f6   isr <- y + z
    MAP_OPCODE(0xf6, op_special_reg_write<false>(isr_));

    // f8   status <- y + z
    MAP_OPCODE(0xf8, op_special_reg_write<false>(status_));

    // fa   mmu_t[y + z] <- x
    MAP_OPCODE(0xfa, BIND_OP(&cpu_state_t::op_store_page_table_entry<false>));

    // fc   mmu_d[y + z] <- x
    MAP_OPCODE(0xfc, BIND_OP(&cpu_state_t::op_store_page_table_entry<true>));

    // Conditional branching:

    // 20   x <- y + z if zero
    MAP_OPCODE(0x20,
               BIND_OP((&cpu_state_t::op_alu<4, false, false, STATUS_ZERO>)));

    // 30   x <- y + z if zero, imm
    MAP_OPCODE(0x30,
               BIND_OP((&cpu_state_t::op_alu<4, false, true, STATUS_ZERO>)));

    // 40   x <- y + z if not zero
    MAP_OPCODE(
        0x40,
        BIND_OP((&cpu_state_t::op_alu<4, false, false, STATUS_ZERO, true>)));

    // 50   x <- y + z if not zero, imm
    MAP_OPCODE(
        0x50,
        BIND_OP((&cpu_state_t::op_alu<4, false, true, STATUS_ZERO, true>)));

    // 60   x <- y + z if neg
    MAP_OPCODE(
        0x60,
        BIND_OP((&cpu_state_t::op_alu<4, false, false, STATUS_NEGATIVE>)));

    // 70   x <- y + z if neg, imm
    MAP_OPCODE(
        0x70, BIND_OP((&cpu_state_t::op_alu<4, false, true, STATUS_NEGATIVE>)));

    // 80   x <- y + z if pos
    MAP_OPCODE(
        0x80,
        BIND_OP((&cpu_state_t::op_alu<4, false, false,
                                      STATUS_NEGATIVE | STATUS_ZERO, true>)));

    // 90   x <- y + z if pos, imm
    MAP_OPCODE(
        0x90,
        BIND_OP((&cpu_state_t::op_alu<4, false, true,
                                      STATUS_NEGATIVE | STATUS_ZERO, true>)));

    // a0   x <- y + z if carry
    MAP_OPCODE(0xa0,
               BIND_OP((&cpu_state_t::op_alu<4, false, false, STATUS_CARRY>)));

    // b0   x <- y + z if carry, imm
    MAP_OPCODE(0xb0,
               BIND_OP((&cpu_state_t::op_alu<4, false, true, STATUS_CARRY>)));

    // c0   x <- y + z if overflow
    MAP_OPCODE(
        0xc0,
        BIND_OP((&cpu_state_t::op_alu<4, false, false, STATUS_OVERFLOW>)));

    // d0   x <- y + z if overflow, imm
    MAP_OPCODE(
        0xd0, BIND_OP((&cpu_state_t::op_alu<4, false, true, STATUS_OVERFLOW>)));

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

void cpu_state_t::cycle()
{
    static int cycle_num = 0;
    LOG(INFO) << endl
              << " -------------------------------- "
              << "cycle " << ++cycle_num << " -------------------------------- "
              << endl;

    LOG(INFO) << "mode=" << (mode_.read() ? "USER" : "KERN");

    const bool cycle_began_as_user = is_user_mode();

    if (cycle_began_as_user)
    {
        mmio_.irq_notify(interrupt_);
        context_switch_to_isr_if({IRQ0, IRQ1, IRQ2, IRQ3, TIME_OUT});
    }

    load_inst_word(inst_);
    LOG(INFO) << "inst=" << inst_.read();

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
    LOG(INFO) << endl << " ---------- inst_op ---------";
    inst_op();

    if (cycle_began_as_user)
    {
        mmio_.irq_notify(interrupt_);
        context_switch_to_isr_if(
            {IRQ0, IRQ1, IRQ2, IRQ3, TIME_OUT, PG_FAULT, RO_FAULT, ILL_INST});
        return;
    }
}

MMIO &cpu_state_t::mmio()
{
    return mmio_;
}

void cpu_state_t::context_switch_to_isr_if(
    const vector<interrupt_signal_t> signals)
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
void cpu_state_t::load_inst_word(reg_t<uint16_t> &reg_x)
{
    const bool user_mode = is_user_mode();
    const uint16_t pc_addr = register_file_.get(PC).read();
    const uint32_t a_phys_bus =
        user_mode ? mmu_.resolve(pc_addr, ptb_.read(), false, false, interrupt_)
                  : pc_addr;

    if (interrupt_.is_signalled({PG_FAULT}))
    {
        return;
    }

    LOG(INFO) << "incrementing pc then loading from "
              << (user_mode ? "user" : "kern") << " code to " << reg_x.name()
              << endl;

    const uint16_t word = mmio_.get_code(user_mode).load_w(a_phys_bus);

    register_file_.get(PC).write(pc_addr + 1);

    if (user_mode)
    {
        exc_addr_.write(pc_addr);
    }

    reg_x.write(word);
}

/// Enable user mode by setting the mode register to 1.
void cpu_state_t::op_set_mode()
{
    if (is_user_mode())
    {
        interrupt_.signal(ILL_INST);
        return;
    }

    mode_.write(1);
}

/// Atomic test and set.
void cpu_state_t::op_ats()
{
    LOG(INFO) << "atomic test and set";
    // Instruction word to decode register parameters.
    const uint16_t inst_word = inst_.read();

    // User data memory.
    ram_t &memory = mmio_.get_data(true);

    // Various registers and values.
    reg_t<uint16_t> &imm = register_file_.get(IMM);
    reg_t<uint16_t> &reg_x = register_file_.get(REG_SEL_X(inst_word));
    uint16_t y = register_file_.get(REG_SEL_Y(inst_word)).read();
    uint16_t z = register_file_.get(REG_SEL_Z(inst_word)).read();

    // Load immediate value, incrementing PC.
    load_inst_word(imm);

    const uint32_t phys_addr =
        mmu_.resolve(y + z, ptb_.read(), true, true, interrupt_);

    if (interrupt_.is_signalled({PG_FAULT, RO_FAULT}))
    {
        return;
    }

    // rx <- mem[ry + rz]
    reg_x.write(memory.load_w(phys_addr));

    // mem[ry + rz] <- imm
    memory.store_w(phys_addr, imm.read());
}

void cpu_state_t::op_invalid()
{
    LOG(ERROR) << "invalid operation." << endl;
    interrupt_.signal(ILL_INST);
}

void cpu_state_t::op_none()
{
    LOG(INFO) << " * * * * op_none * * * *\n";
}

bool cpu_state_t::is_user_mode() const
{
    return status_.read() & 0x8;
}
