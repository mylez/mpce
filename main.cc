#include "control.h"
#include "cpu_state.h"

#include <cstdio>

int main()
{
    MPCE::CPUState cpu_state;
    cpu_state.cycle();

    uint16_t u = 0x100a;

    int16_t s = static_cast<int16_t>(u);

    printf("%x\n", static_cast<int>(s));

    return 0;
}
