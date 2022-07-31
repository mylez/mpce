#include "cpu_state.h"
#include "io_serial.h"

#include <cstdio>
#include <stack>

using namespace std;

int main(int argc, char *argv[])
{
    MPCE::CPUState cpu_state;

    // Load immediate to r7:
    // 32   x <- y ^ z, imm
    uint16_t inst_load_imm = 0x3200 | 1 | 7 << 3;
    cpu_state.mmio().get_code(false).store(0, inst_load_imm);
    cpu_state.mmio().get_code(false).store(1, 'M');

    // Store r1 to r0 + r7:
    // b4   mem_b_kern[y + z] <- x, imm
    uint16_t inst_io_store = 0xb400 | 1 | 7 << 6;
    cpu_state.mmio().get_code(false).store(2, inst_io_store);
    cpu_state.mmio().get_code(false).store(3, 0xf000);

    cpu_state.mmio().serial_interface().start_console();

    for (int i = 0; i < 2; i++)
    {
        this_thread::sleep_for(chrono::seconds(1));
        cpu_state.cycle();
    }

    cpu_state.mmio().serial_interface().join_console();
}