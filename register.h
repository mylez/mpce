#pragma once

#include <cstdint>
#include <iostream>
#include <string>

#include <glog/glog.h>

using namespace std;

namespace MPCE
{

/// @brief
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

/// @brief
const unsigned int REGISTER_FILE_SIZE = 8;

/// @brief
/// @tparam register_t
template <typename register_t> class Register
{
  private:
    /// The data stored in this register.
    register_t data_;

    /// High bits in mask are disabled in the register.
    register_t mask_;

    /// Name of register.
    const string name_;

  public:
    /// @brief
    /// @param name
    /// @param mask
    Register(const string name, const register_t mask = {})
        : data_{}, mask_(mask), name_{name}
    {
    }

    /// @brief
    /// @return
    register_t read() const
    {
        LOG(INFO) << "reading " << static_cast<uint32_t>(data_)
                  << " from register " << name_;
        return data_;
    }

    /// @brief
    /// @param data
    void write(const register_t &data)
    {
        LOG(INFO) << "writing " << static_cast<uint32_t>(data)
                  << " to register " << name_;
        data_ = data & ~mask_;
    }

    /// @brief
    /// @return
    string name() const
    {
        return name_;
    }
};

/// @brief
class RegisterFile
{
  private:
    /// @brief
    Register<uint16_t> registers_[REGISTER_FILE_SIZE] = {
        {"r0", 0xffff}, {"r1"}, {"r2"}, {"r3"},
        {"fp"},         {"sp"}, {"pc"}, {"imm"}};

  public:
    /// @brief
    /// @param index
    /// @return
    Register<uint16_t> &get(const uint8_t index)
    {
        return registers_[index & 0x07];
    }
};

} // namespace MPCE