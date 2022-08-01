#pragma once

#include <cstdint>
#include <iostream>
#include <string>

using namespace std;

namespace MPCE
{

enum RegisterIndex
{
    R0 = 0,
    R1 = 1,
    R2 = 2,
    R3 = 3,
    R4 = 4,
    SP = 5,
    PC = 6,
    IMM = 7
};

const unsigned int REGISTER_FILE_SIZE = 8;

template <typename DataType> class Register
{
  private:
    /// The data stored in this register.
    DataType data_;

    /// High bits in mask are disabled in the register.
    DataType mask_;

    /// Name of register.
    const string name_;

  public:
    Register(const string name, const DataType mask = {})
        : data_{}, mask_(mask), name_{name}
    {
        cout << "initializing register " << name_ << endl;
    }

    DataType read() const
    {
        printf("reading 0x%x from register %s\n", data_, name_.c_str());
        return data_;
    }

    void write(const DataType &data)
    {
        printf("writing 0x%x to register %s\n", data, name_.c_str());
        data_ = data & ~mask_;
    }

    string name() const
    {
        return name_;
    }
};

class RegisterFile
{
  private:
    Register<uint16_t> registers_[REGISTER_FILE_SIZE] = {
        {"r0", 0xffff}, {"r1"}, {"r2"}, {"r3"},
        {"fp"},         {"sp"}, {"pc"}, {"imm"}};

  public:
    Register<uint16_t> &get(const uint8_t index)
    {
        return registers_[index & 0x07];
    }
};

} // namespace MPCE