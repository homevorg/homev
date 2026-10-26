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

#include "test.hpp"

#include <cassert>

using hmv::Address;
using hmv::generator::mcversi::ProtoOperation;
using hmv::generator::mcversi::Randomizer;
using hmv::generator::mcversi::Test;

Randomizer::Randomizer(ThreadCount thread_count,
                       ByteCount usable_memory_size,
                       ByteCount stride,
                       ByteCount chunk_size,
                       ByteCount hole_size,
                       TestSize nop_max_seq_size,
                       const OperationMix& mix)
    : stride_mask{~static_cast<Address>(stride - 1)}, hole_size{hole_size},
      mix{mix}, addr_dist{0, chunk_size - 1},
      chunk_dist{0, usable_memory_size / chunk_size - 1},
      tid_dist{ThreadID{0}, static_cast<ThreadID>(thread_count - 1)},
      nop_dist{1, nop_max_seq_size} {}

ProtoOperation Randomizer::operator()(URNG& urng) {
    switch (auto type = mix(urng)) {
    case ProtoOperation::READ:
    case ProtoOperation::READ_ADDR_DEP:
    case ProtoOperation::WRITE:
        return {type, address(urng), tid_dist(urng)};
    case ProtoOperation::FENCE:
        return {type, 0, tid_dist(urng)};
    case ProtoOperation::DELAY:
        return {type, nop_dist(urng), tid_dist(urng)};
    default:
        assert(false);
        return {};
    }
}

ProtoOperation Randomizer::operator()(
    URNG& urng, const std::unordered_set<Address>& addresses) {
    switch (auto type = mix(urng)) {
    case ProtoOperation::READ:
    case ProtoOperation::READ_ADDR_DEP:
    case ProtoOperation::WRITE:
        return {type, address_from(urng, addresses), tid_dist(urng)};
    case ProtoOperation::FENCE:
        return {type, 0, tid_dist(urng)};
    case ProtoOperation::DELAY:
        return {type, nop_dist(urng), tid_dist(urng)};
    default:
        assert(false);
        return {};
    }
}

Address Randomizer::address(URNG& urng) {
    return (chunk_dist(urng) * hole_size + addr_dist(urng)) & stride_mask;
}

Address Randomizer::address_from(URNG& urng,
                                 const std::unordered_set<Address>& addresses) {
    // HMV note: a sensible implementation to select an address from a set would
    // be something like the code commented out below...
    // auto dist =
    //     std::uniform_int_distribution<size_t>{0ul, addresses.size() - 1};
    // return *std::next(addresses.begin(), dist(urng));
    // ...but McVerSi uses a weird implementation which can lead to an address
    // outside the set to be chosen. Not choosing a fitaddr where expected may
    // have non-negligible impact, so we're keeping this dubious implementation
    // here to be faithful to the original.
    // See RandInstTest::MakeRandom(...) function (line 3768) in
    // https://www.marcoelver.com/files/mcversi_gem5.patch
    auto addr = Address{0};
    for (auto i = 0u; i < 1000; ++i) {
        addr = address(urng);
        if (addresses.count(addr)) {
            return addr;
        }
    }
    return addr;
}

Test Test::make_random(size_t size, Randomizer& random, URNG& urng) {
    auto test = Test{1, std::vector<ProtoOperation>{size}, {}, 0};
    for (auto& op : test.operations) {
        op = random(urng);
    }
    return test;
}
