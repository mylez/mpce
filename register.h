#pragma once

#include <cstdint>
#include <iostream>
#include <string>

#include <glog/logging.h>

using namespace std;

/// @brief
enum register_index
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
/// @tparam dtype_t
template <typename dtype_t> class reg_t
{
  private:
    /// The data stored in this register.
    dtype_t data_;

    /// High bits in mask are disabled in the register.
    dtype_t mask_;

    /// Name of register.
    const string name_;

  public:
    /// @brief
    /// @param name
    /// @param mask
    reg_t(const string name, const dtype_t mask = {})
        : data_{}, mask_(mask), name_{name}
    {
    }

    /// @brief
    /// @return
    dtype_t read() const
    {
        LOG(INFO) << hex << "reading " << static_cast<uint32_t>(data_)
                  << " from register " << name_;
        return data_;
    }

    /// @brief
    /// @param data
    void write(const dtype_t &data)
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
class reg_file_t
{
  private:
    /// @brief
    reg_t<uint16_t> registers_[REGISTER_FILE_SIZE] = {
        {"r0", 0xffff}, {"r1"}, {"r2"}, {"r3"},
        {"fp"},         {"sp"}, {"pc"}, {"imm"}};

  public:
    /// @brief
    /// @param index
    /// @return
    reg_t<uint16_t> &get(const uint8_t index)
    {
        return registers_[index & 0x07];
    }
};
