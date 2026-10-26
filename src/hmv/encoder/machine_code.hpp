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

#ifndef __HMV_ENCODER_MACHINE_CODE_HPP__
#define __HMV_ENCODER_MACHINE_CODE_HPP__

#include <cassert>
#include <memory>

#include "hmv/core/register_allocator.hpp"
#include "hmv/core/test.hpp"
#include "hmv/core/types.hpp"
#include "hmv/utility/byte.hpp"

namespace hmv::encoder {
    class MachineCode {
     public:
        MachineCode(ISA isa, MemoryTraits traits);
        ByteSequence encode(const std::vector<Address>& addresses) const;
        std::vector<ByteSequence> encode(Test& test);
        ByteCount max_thread_size(TestSize op_count) const;

     private:
        class Concept {
         public:
            virtual ~Concept() = default;
            virtual ByteSequence encode(
                const std::vector<Address>& addresses) const = 0;
            virtual std::vector<ByteSequence> encode(Test& test) = 0;
            virtual ByteCount max_thread_size(TestSize op_count) const = 0;
        };

        template <typename E>
        class Model final : public Concept {
         public:
            Model(E&& encoder);
            ByteSequence encode(
                const std::vector<Address>& addresses) const override;
            std::vector<ByteSequence> encode(Test& test) override;
            ByteCount max_thread_size(TestSize op_count) const override;

         private:
            E encoder;
            RegisterAllocator register_allocator;

            void load(ByteSequence& sequence,
                      Operation op,
                      Address address) const;
            void store(ByteSequence& sequence,
                       Operation op,
                       Address address) const;
            void add(ByteSequence& sequence,
                     Operation op,
                     Address address) const;
            void exor(ByteSequence& sequence,
                      Operation op,
                      Address address) const;
            void apply_usage(ByteSequence& sequence,
                             Operation op,
                             Address address,
                             Register reg) const;
        };

        std::unique_ptr<Concept> model;
    };

    template <typename E>
    inline MachineCode::Model<E>::Model(E&& encoder)
        : encoder{std::move(encoder)} {}

    template <typename E>
    inline ByteSequence MachineCode::Model<E>::encode(
        const std::vector<Address>& addresses) const {
        return encoder.encode(addresses);
    }

    template <typename E>
    inline std::vector<ByteSequence> MachineCode::Model<E>::encode(Test& test) {
        register_allocator.randomize(test);
        auto threads = std::vector<ByteSequence>{};
        threads.reserve(test.threads.size());
        for (auto& thread : test.threads) {
            auto& sequence = threads.emplace_back();
            sequence.reserve(max_thread_size(thread.size()));
            for (auto op : thread) {
                switch (op.type) {
                case Operation::LOAD:
                    load(sequence, op, test.addresses[op.address]);
                    break;
                case Operation::STORE:
                    store(sequence, op, test.addresses[op.address]);
                    break;
                case Operation::FENCE:
                    encoder.encode_fence(sequence);
                    break;
                case Operation::NOP:
                    encoder.encode_nop(sequence, op.value);
                    break;
                case Operation::ADD:
                    add(sequence, op, test.addresses[op.address]);
                    break;
                case Operation::XOR:
                    exor(sequence, op, test.addresses[op.address]);
                    break;
                default:
                    assert(false);
                    break;
                }
            }
            encoder.encode_ret(sequence);
        }
        return threads;
    }

    template <typename E>
    inline ByteCount MachineCode::Model<E>::max_thread_size(
        TestSize op_count) const {
        return (op_count + 1) * encoder.max_operation_size();
    }

    template <typename E>
    inline void MachineCode::Model<E>::load(ByteSequence& sequence,
                                            Operation op,
                                            Address address) const {
        auto src1 = encoder.map_register(op.src1);
        if (op.src1.usage() == Register::DEFAULT) {
            encoder.encode_move_address(sequence, src1, address);
        } else {
            apply_usage(sequence, op, address, op.src1);
        }
        encoder.encode_load(
            sequence, encoder.map_register(op.dest), src1, address);
    }

    template <typename E>
    inline void MachineCode::Model<E>::store(ByteSequence& sequence,
                                             Operation op,
                                             Address address) const {
        auto src1 = encoder.map_register(op.src1);
        auto src2 = encoder.map_register(op.src2);
        if (op.src1.usage() == Register::DEFAULT) {
            encoder.encode_move_address(sequence, src1, address);
        } else {
            apply_usage(sequence, op, address, op.src1);
        }
        if (op.src2.usage() == Register::DEFAULT) {
            encoder.encode_move_value(sequence, src2, op.value);
        } else {
            apply_usage(sequence, op, address, op.src2);
        }
        encoder.encode_store(sequence, src1, src2, address);
    }

    template <typename E>
    inline void MachineCode::Model<E>::add(ByteSequence& sequence,
                                           Operation op,
                                           Address address) const {
        apply_usage(sequence, op, address, op.src1);
        apply_usage(sequence, op, address, op.src2);
        encoder.encode_add(sequence,
                           encoder.map_register(op.dest),
                           encoder.map_register(op.src1),
                           encoder.map_register(op.src2));
    }

    template <typename E>
    inline void MachineCode::Model<E>::exor(ByteSequence& sequence,
                                            Operation op,
                                            Address address) const {
        apply_usage(sequence, op, address, op.src1);
        apply_usage(sequence, op, address, op.src2);
        encoder.encode_exor(sequence,
                            encoder.map_register(op.dest),
                            encoder.map_register(op.src1),
                            encoder.map_register(op.src2));
    }

    template <typename E>
    inline void MachineCode::Model<E>::apply_usage(ByteSequence& sequence,
                                                   Operation op,
                                                   Address address,
                                                   Register reg) const {
        switch (reg.usage()) {
        case Register::ASSIGN_ADDRESS:
            encoder.encode_move_address(
                sequence, encoder.map_register(reg), address);
            break;
        case Register::ASSIGN_VALUE:
            encoder.encode_move_value(
                sequence, encoder.map_register(reg), op.value);
            break;
        default:
            break;
        }
    }
} // namespace hmv::encoder

#endif /* __HMV_ENCODER_MACHINE_CODE_HPP__ */
