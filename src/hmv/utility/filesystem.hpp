// Copyright (C) 2025-2026 Marleson Graf

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#ifndef __HMV_UTILITY_FILESYSTEM_HPP__
#define __HMV_UTILITY_FILESYSTEM_HPP__

#include "hmv/utility/type.hpp"
#include <filesystem>
#include <fstream>
#include <type_traits>

namespace hmv {
    std::ifstream open_input_file(const std::filesystem::path& filename,
                                  std::ios::openmode mode = {});
    std::ofstream open_output_file(const std::filesystem::path& filename,
                                   std::ios::openmode mode = {});
    void serialize(std::ofstream& file, const std::string& value);
    template <typename T>
    void serialize(std::ofstream& file, T value);
    template <typename T>
    T deserialize(std::ifstream& file);

    template <typename T>
    inline void serialize(std::ofstream& file, T value) {
        if constexpr (std::is_integral_v<T> || std::is_enum_v<T>) {
            file.write(reinterpret_cast<const char*>(&value), sizeof(T));
        } else if constexpr (IsContainer<T>::value) {
            auto size = value.size();
            serialize(file, size);
            file.write(reinterpret_cast<const char*>(value.data()),
                       size * sizeof(typename T::value_type));
        } else {
            static_assert(false, "unsupported serialization");
        }
    }

    template <typename T>
    inline T deserialize(std::ifstream& file) {
        auto value = T{};
        if constexpr (std::is_integral_v<T> || std::is_enum_v<T>) {
            file.read(reinterpret_cast<char*>(&value), sizeof(T));
        } else if constexpr (std::is_same_v<T, std::string>) {
            auto size = deserialize<std::string::size_type>(file);
            value.resize(size);
            file.read(value.data(), size);
        } else if constexpr (IsContainer<T>::value) {
            auto size = deserialize<typename T::size_type>(file);
            value.resize(size);
            file.read(reinterpret_cast<char*>(value.data()),
                      size * sizeof(typename T::value_type));
        } else {
            static_assert(false, "unsupported unserialization");
        }
        return value;
    }
} // namespace hmv

#endif /* __HMV_UTILITY_FILESYSTEM_HPP__ */
