#include "control.h"
#include "cpu_state.h"

#include <cstdio>

int main(int argc, char *argv[])
{
    MPCE::CPUState cpu_state;
    cpu_state.cycle();

    return 0;
}
