#include "cpu_state.h"

#include <cstdio>

int main(int argc, char *argv[])
{
    MPCE::CPUState cpu_state;
    cpu_state.cycle();

    return 0;

    using clock = std::chrono::high_resolution_clock;

    long sum_tick = 0;
    int n = 1;
    int i = 0;

    while (true)
    {
        clock::time_point t1 = clock::now();
        sum_tick = 0;
        while (true)
        {
            clock::time_point t2 = clock::now();

            clock::duration d = t2 - t1;

            if (d >
                std::chrono::milliseconds(16) - std::chrono::nanoseconds(100))
                break;
        }

        clock::time_point t3 = clock::now();
        clock::duration d = t3 - t1;

        sum_tick += d.count();

        if (i++ % 1 == 0)
            cout << "avg: " << (sum_tick / n) << endl;
    }

    return 0;
}
