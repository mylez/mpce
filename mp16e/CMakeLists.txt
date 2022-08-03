cmake_minimum_required(VERSION 3.22)
project(mp16e)
add_executable(mpce main.cc)
target_compile_features(mpce PRIVATE cxx_std_17)