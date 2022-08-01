cmake_minimum_required(VERSION 3.22)
project(mp16e)
add_executable(mpce main.cc)
target_compile_features(mpce PRIVATE cxx_std_20)
target_compile_options(mpce PRIVATE -Wall -Wextra -Wpedantic)
add_library(mpce_lib cpu_state.cc cpu_state.h interrupt.h io_serial.h io.h
    memory.cc memory.h mmio.cc mmio.h mmu.cc mmu.h register.cc register.h)