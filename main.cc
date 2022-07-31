#include "cpu_state.h"
#include "io_serial.h"

#include <cstdio>

int main(int argc, char *argv[])
{
    MPCE::CPUState cpu_state;

    cpu_state.cycle();
    cpu_state.cycle();
    cpu_state.cycle();
}