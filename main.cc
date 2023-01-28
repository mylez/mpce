#include "cpu_state.h"
#include "io_serial.h"

#include <cstdio>
#include <stack>

#include <gflags/gflags.h>
#include <glog/logging.h>

using namespace std;

int main(int argc, char *argv[])
{
    google::InitGoogleLogging(argv[0]);

    FLAGS_logtostderr = 1;
    FLAGS_stderrthreshold = 0;
    FLAGS_minloglevel = 0;

    mpce::cpu_state_t cpu_state;

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

    // Store r1 to r0 + r7:
    // b4   mem_b_kern[y + z] <- x, imm
    uint16_t inst_ats = 0x6c00 | 1 | 2 << 3 | 3 << 6;
    cpu_state.mmio().get_code(false).store(4, inst_ats);
    cpu_state.mmio().get_code(false).store(5, 1);

    for (int i = 0; i < 3; i++)
    {
        cpu_state.cycle();
    }
}