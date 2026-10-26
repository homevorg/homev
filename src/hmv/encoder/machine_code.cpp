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

#include "machine_code.hpp"

#include "component/a64.hpp"
#include "component/rv64.hpp"

using hmv::Address;
using hmv::ByteCount;
using hmv::ByteSequence;
using hmv::Test;
using hmv::encoder::MachineCode;

MachineCode::MachineCode(ISA isa, MemoryTraits traits) {
    switch (isa) {
    case ISA::RISCV:
        model = std::make_unique<Model<RV64>>(RV64{traits});
        break;
    case ISA::ARM:
        model = std::make_unique<Model<A64>>(A64{traits});
        break;
    }
    assert(model);
}

ByteSequence MachineCode::encode(const std::vector<Address>& addresses) const {
    return model->encode(addresses);
}

std::vector<ByteSequence> MachineCode::encode(Test& test) {
    return model->encode(test);
}

ByteCount MachineCode::max_thread_size(TestSize thread_size) const {
    return model->max_thread_size(thread_size);
}
