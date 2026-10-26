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

#include "biasing.hpp"

#include <algorithm>
#include <limits>
#include <set>

#include "hmv/utility/math.hpp"
#include "hmv/utility/type.hpp"

using hmv::Address;
using hmv::AddressCount;
using hmv::BitCount;
using hmv::generator::Biasing;

Biasing::Biasing(MemoryTraits memory_traits,
                 CacheTraits cache_traits,
                 bool sbc,
                 Seed seed)
    : offset_bits(cache_traits.offset_size()),
      index_bits(cache_traits.index_size()),
      tag_bits(memory_traits.address_size - index_bits - offset_bits),
      min_abc(fast_uint_log2(to_underlying(memory_traits.word_size))),
      urng{seed}, tag_shift{static_cast<BitCount>(index_bits + offset_bits)},
      sbc{sbc}, tag_dist{0, (1u << tag_bits) - 1},
      index_dist{0, (1u << index_bits) - 1} {}

std::vector<Address> Biasing::operator()(AddressCount s,
                                         const CBC& cbc,
                                         BitCount abc) {
    last_abc = validate_abc(abc);
    last_cbc = validate_cbc(cbc, s, last_abc);
    auto max_offset = (1u << (offset_bits - last_abc)) - 1;
    set_param<Address>(offset_dist, 0, max_offset);
    auto sets = std::map<Address, AddressCount>();
    auto addresses = AddressSet{~max_offset, sbc};
    while (sets.size() < last_cbc.k) {
        auto index = index_dist(urng);
        if (sets.insert({index, last_cbc.x - 1}).second) {
            addresses.insert(select_address(index, last_abc));
        }
    }
    if (addresses.size() < s) {
        set_param<Address>(set_dist, 0, sets.size() - 1);
        auto index = select_index(sets);
        while (sets[index] > 0) {
            insert(sets, addresses, index);
        }
        while (addresses.size() < s) {
            insert(sets, addresses, select_index(sets));
        }
    }
    return addresses.extract();
}

std::vector<Address> Biasing::operator()(Range& range) {
    set_param(abc_dist, min_abc, offset_bits);
    auto abc = abc_dist(urng);
    auto s = range(urng);
    return operator()(s, random_cbc(s, abc), abc);
}

std::vector<Address> Biasing::operator()(Range& range, BitCount abc) {
    auto s = range(urng);
    return operator()(s, random_cbc(s, abc), abc);
}

void Biasing::insert(std::map<Address, AddressCount>& sets,
                     AddressSet& addresses,
                     Address index) {
    if (addresses.insert(select_address(index, last_abc))) {
        if (--sets[index] == 0) {
            sets.erase(index);
            set_param<Address>(set_dist, 0, sets.size() - 1);
        }
    }
}

Biasing::CBC Biasing::random_cbc(AddressCount s, BitCount abc) {
    auto cbc = Biasing::CBC{};
    set_param<AddressCount>(k_dist, 1, s);
    do {
        cbc.k = k_dist(urng);
        set_param<AddressCount>(x_dist, s / cbc.k, s - cbc.k);
        cbc.x = x_dist(urng);
    } while (!feasible(cbc, abc));
    return cbc;
}

Address Biasing::select_address(Address index, BitCount abc) {
    return (index << offset_bits) | (tag_dist(urng) << tag_shift)
           | (offset_dist(urng) << abc);
}

Address Biasing::select_index(std::map<Address, AddressCount>& sets) {
    return std::next(sets.begin(), set_dist(urng))->first;
}

BitCount Biasing::validate_abc(BitCount abc) const {
    return std::min(offset_bits, std::max(min_abc, abc));
}

Biasing::CBC Biasing::validate_cbc(CBC cbc,
                                   AddressCount s,
                                   BitCount abc) const {
    cbc.k = std::min({cbc.k, s, static_cast<AddressCount>(1u << index_bits)});
    cbc.x = std::min(cbc.x, static_cast<AddressCount>(s - cbc.k + 1));
    if (auto max_set_size = 1u << (tag_bits + offset_bits - abc);
        max_set_size < std::numeric_limits<AddressCount>::max()) {
        cbc.x = std::min(cbc.x, max_set_size);
    }
    cbc.x = std::max(cbc.x, static_cast<AddressCount>((s + cbc.k - 1) / cbc.k));
    return cbc;
}

Biasing::AddressSet::AddressSet(Address mask, bool sbc)
    : mask{mask}, sbc{sbc} {}

bool Biasing::AddressSet::insert(Address address) {
    if (sbc && !masked_addresses.insert(address & mask).second) {
        return false;
    }
    return addresses.insert(address).second;
}

std::vector<Address> Biasing::AddressSet::extract() {
    return {addresses.begin(), addresses.end()};
}

AddressCount Biasing::AddressSet::size() const {
    return addresses.size();
}
