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

#ifndef __HMV_GENERATOR_BIASING_HPP__
#define __HMV_GENERATOR_BIASING_HPP__

#include <map>
#include <random>
#include <set>
#include <vector>

#include "hmv/core/random.hpp"
#include "hmv/core/random_range.hpp"
#include "hmv/core/types.hpp"

namespace hmv::generator {
    class Biasing {
     public:
        struct CBC {
            AddressCount k;
            AddressCount x;
        };

        Biasing(MemoryTraits memory_traits,
                CacheTraits cache_traits,
                bool sbc = false,
                Seed seed = std::random_device{}());
        std::vector<Address> operator()(AddressCount s,
                                        const CBC& cbc,
                                        BitCount abc);
        std::vector<Address> operator()(Range& range, BitCount abc);
        std::vector<Address> operator()(Range& range);
        CBC cbc() const;
        BitCount abc() const;
        bool feasible(const CBC& cbc, BitCount abc) const;

     private:
        BitCount offset_bits;
        BitCount index_bits;
        BitCount tag_bits;
        BitCount min_abc;
        BitCount tag_shift;
        bool sbc;
        URNG urng;
        std::uniform_int_distribution<Address> tag_dist;
        std::uniform_int_distribution<Address> index_dist;
        std::uniform_int_distribution<Address> offset_dist;
        std::uniform_int_distribution<Address> set_dist;
        std::bernoulli_distribution bool_dist{0.5};
        std::uniform_int_distribution<BitCount> abc_dist;
        std::uniform_int_distribution<AddressCount> k_dist;
        std::uniform_int_distribution<AddressCount> x_dist;
        CBC last_cbc;
        BitCount last_abc;

        class AddressSet {
         public:
            AddressSet(Address mask, bool sbc);
            bool insert(Address address);
            std::vector<Address> extract();
            AddressCount size() const;

         private:
            Address mask;
            bool sbc;
            std::set<Address> addresses;
            std::set<Address> masked_addresses;
        };

        void insert(std::map<Address, AddressCount>& sets,
                    AddressSet& addresses,
                    Address index);
        CBC random_cbc(AddressCount s, BitCount abc);
        Address select_address(Address index, BitCount abc);
        Address select_index(std::map<Address, AddressCount>& sets);
        BitCount validate_abc(BitCount abc) const;
        CBC validate_cbc(CBC cbc, AddressCount s, BitCount abc) const;
    };

    inline BitCount Biasing::abc() const {
        return last_abc;
    }

    inline Biasing::CBC Biasing::cbc() const {
        return last_cbc;
    }

    inline bool Biasing::feasible(const CBC& cbc, BitCount abc) const {
        auto max_set_size = 1ul << (tag_bits + (sbc ? 0 : offset_bits - abc));
        return abc <= offset_bits && cbc.k > 0 && cbc.k <= (1ul << index_bits)
               && cbc.x > 0 && cbc.x <= max_set_size;
    }
} // namespace hmv::generator

#endif /* __HMV_GENERATOR_BIASING_HPP__ */
