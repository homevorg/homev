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

#include "operation.hpp"

using hmv::Operation;

Operation::Type hmv::to_operation_type(const std::string& type) {
    switch (type.size()) {
    case 3:
        if (type == "NOP") {
            return Operation::NOP;
        }
        if (type == "ADD") {
            return Operation::ADD;
        }
        if (type == "XOR") {
            return Operation::XOR;
        }
        break;
    case 4:
        if (type == "LOAD") {
            return Operation::LOAD;
        }
        break;
    case 5:
        if (type == "STORE") {
            return Operation::STORE;
        }
        break;
    case 6:
        if (type == "FENCE") {
            return Operation::FENCE;
        }
        break;
    }
    return Operation::NONE;
}

std::string hmv::to_string(Operation::Type type) {
    switch (type) {
    case Operation::LOAD:
        return "LOAD";
    case Operation::STORE:
        return "STORE";
    case Operation::FENCE:
        return "FENCE";
    case Operation::NOP:
        return "NOP";
    case Operation::ADD:
        return "ADD";
    case Operation::XOR:
        return "XOR";
    case Operation::NONE:
        return "NONE";
    }
    __builtin_unreachable();
}

std::ostream& hmv::operator<<(std::ostream& out, Operation::Type type) {
    return out << to_string(type);
}
