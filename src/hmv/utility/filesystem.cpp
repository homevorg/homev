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

#include "filesystem.hpp"

std::ifstream hmv::open_input_file(const std::filesystem::path& filename,
                                   std::ios_base::openmode mode) {
    return std::ifstream{filename, mode};
}

std::ofstream hmv::open_output_file(const std::filesystem::path& filename,
                                    std::ios_base::openmode mode) {
    if (filename.empty()) {
        return {};
    }
    if (filename.has_parent_path()) {
        if (auto dir = filename.parent_path(); !std::filesystem::exists(dir)) {
            std::filesystem::create_directories(dir);
        }
    }
    return std::ofstream{filename, mode};
}

void hmv::serialize(std::ofstream& file, const std::string& value) {
    auto size = value.size();
    serialize(file, size);
    file.write(value.data(), size);
}
