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
    FP = 4,
    SP = 5,
    PC = 6,
    IMM = 7
};

const unsigned int REGISTER_FILE_SIZE = 8;

template <typename DataType> class Register
{
  private:
    /// @brief The data stored in this register.
    DataType data_;

    /// @brief High bits in mask are disabled in the register.
    DataType mask_;

    /// @brief Name of register.
    const std::string name_;

  public:
    Register(const std::string name, const DataType mask = {})
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

    std::string name() const
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
        if (index >= REGISTER_FILE_SIZE)
        {
            // Error condition.
            return registers_[R0];
        }

        return registers_[index];
    }
};

} // namespace MPCE