#include "cpu_state.h"
#include "io_serial.h"

#include <cstdio>

int main(int argc, char *argv[])
{
    MPCE::IOSerialInterface io_serial;

    io_serial.start_console();
    io_serial.join_console();
}