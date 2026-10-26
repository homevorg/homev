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

#include <nanobind/make_iterator.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/array.h>
#include <nanobind/stl/bind_map.h>
#include <nanobind/stl/bind_vector.h>
#include <nanobind/stl/filesystem.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/set.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>

#include "hmv/checker/checker.hpp"
#include "hmv/core/event.hpp"
#include "hmv/core/operation.hpp"
#include "hmv/core/random_mix.hpp"
#include "hmv/core/register.hpp"
#include "hmv/core/test.hpp"
#include "hmv/core/time_counter.hpp"
#include "hmv/core/trace.hpp"
#include "hmv/core/types.hpp"
#include "hmv/coverage/metric.hpp"
#include "hmv/generator/component/biasing.hpp"
#include "hmv/generator/component/chaining.hpp"
#include "hmv/generator/component/random_address.hpp"
#include "hmv/generator/component/random_thread.hpp"
#include "hmv/generator/htg.hpp"
#include "hmv/generator/mcversi.hpp"
#include "hmv/generator/sca.hpp"
#include "hmv/platform/component/signals.hpp"
#include "hmv/platform/output.hpp"
#include "hmv/platform/traits.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace hmv;

NB_MODULE(_hmv_internal, m) {
    nb::enum_<WordSize>(m, "WordSize")
        .value("DWORD", WordSize::DWORD)
        .value("QWORD", WordSize::QWORD)
        .export_values();
    nb::enum_<ISA>(m, "ISA")
        .value("ARM", ISA::ARM)
        .value("RISCV", ISA::RISCV)
        .export_values();

    nb::class_<MemoryTraits>(m, "MemoryTraits")
        .def(nb::init<BitCount, WordSize>(), "address_size"_a, "word_size"_a)
        .def_rw("address_size", &MemoryTraits::address_size)
        .def_rw("word_size", &MemoryTraits::word_size);

    nb::class_<CacheTraits>(m, "CacheTraits")
        .def(nb::init<ByteCount, ByteCount, BlockCount>(),
             "size"_a,
             "block_size"_a,
             "associativity"_a)
        .def_rw("size", &CacheTraits::size)
        .def_rw("block_size", &CacheTraits::block_size)
        .def_rw("associativity", &CacheTraits::associativity)
        .def("index_size", &CacheTraits::index_size)
        .def("offset_size", &CacheTraits::offset_size);

    auto reg = nb::class_<Register>(m, "Register");
    nb::enum_<Register::Name>(reg, "Name")
        .value("R1", Register::R1)
        .value("R2", Register::R2)
        .value("R3", Register::R3)
        .value("R4", Register::R4)
        .value("R5", Register::R5)
        .value("R6", Register::R6)
        .value("R7", Register::R7)
        .value("R8", Register::R8)
        .value("NONE", Register::NONE)
        .export_values();
    nb::enum_<Register::Usage>(reg, "Usage")
        .value("DEFAULT", Register::DEFAULT)
        .value("ASSIGN_ADDRESS", Register::ASSIGN_ADDRESS)
        .value("ASSIGN_VALUE", Register::ASSIGN_VALUE)
        .value("PRESERVE", Register::PRESERVE)
        .export_values();
    reg.def(nb::init<Register::Name, Register::Usage>(),
            "name"_a = Register::NONE,
            "usage"_a = Register::DEFAULT);

    auto operation = nb::class_<Operation>(m, "Operation");
    nb::enum_<Operation::Type>(operation, "Type")
        .value("LOAD", Operation::Type::LOAD)
        .value("STORE", Operation::Type::STORE)
        .value("FENCE", Operation::Type::FENCE)
        .value("NOP", Operation::Type::NOP)
        .value("ADD", Operation::Type::ADD)
        .value("XOR", Operation::Type::XOR)
        .value("NONE", Operation::Type::NONE)
        .export_values();
    operation
        .def(nb::init<Data,
                      AddressCount,
                      Operation::Type,
                      Register,
                      Register,
                      Register>(),
             "type"_a,
             "address"_a = AddressCount{},
             "data"_a = Data{},
             "dest"_a = Register{},
             "src1"_a = Register{},
             "src2"_a = Register{})
        .def(nb::init<Data,
                      AddressCount,
                      Operation::Type,
                      Register::Name,
                      Register::Name,
                      Register::Name>(),
             "type"_a,
             "address"_a = AddressCount{},
             "data"_a = Data{},
             "dest"_a = Register::NONE,
             "src1"_a = Register::NONE,
             "src2"_a = Register::NONE)
        .def_rw("value", &Operation::value)
        .def_rw("address", &Operation::address)
        .def_rw("type", &Operation::type)
        .def_rw("dest", &Operation::dest)
        .def_rw("src1", &Operation::src1)
        .def_rw("src2", &Operation::src2);

    nb::class_<Load, Operation>(m, "Load")
        .def(nb::init<AddressCount, Register, Register>(),
             "address"_a = Operation::NONE,
             "dest"_a = Register{},
             "src1"_a = Register{})
        .def(nb::init<AddressCount, Register::Name, Register::Name>(),
             "address"_a,
             "dest"_a = Register::NONE,
             "src1"_a = Register::NONE);
    nb::class_<Store, Operation>(m, "Store")
        .def(nb::init<AddressCount, Data, Register, Register>(),
             "address"_a,
             "value"_a,
             "src1"_a = Register{},
             "src2"_a = Register{})
        .def(nb::init<AddressCount, Data, Register::Name, Register::Name>(),
             "address"_a,
             "value"_a,
             "src1"_a = Register::NONE,
             "src2"_a = Register::NONE);
    nb::class_<Add, Operation>(m, "Add")
        .def(nb::init<Register, Register, Register, AddressCount, Data>(),
             "dest"_a,
             "src1"_a,
             "src2"_a,
             "address"_a = 0,
             "value"_a = 0)
        .def(nb::init<Register::Name,
                      Register::Name,
                      Register::Name,
                      AddressCount,
                      Data>(),
             "dest"_a,
             "src1"_a,
             "src2"_a,
             "address"_a = 0,
             "value"_a = 0);
    nb::class_<Xor, Operation>(m, "Xor")
        .def(nb::init<Register, Register, Register, AddressCount, Data>(),
             "dest"_a,
             "src1"_a,
             "src2"_a,
             "address"_a = 0,
             "value"_a = 0)
        .def(nb::init<Register::Name,
                      Register::Name,
                      Register::Name,
                      AddressCount,
                      Data>(),
             "dest"_a,
             "src1"_a,
             "src2"_a,
             "address"_a = 0,
             "value"_a = 0);
    nb::class_<Fence, Operation>(m, "Fence").def(nb::init<>());
    nb::class_<Nop, Operation>(m, "Nop").def(nb::init<Data>(), "amount"_a);

    nb::class_<OperationMix>(m, "OperationMix")
        .def(nb::init<const OperationMix::RatioArray&>(), "ratios"_a)
        .def_ro("ratios", &OperationMix::ratios);
    nb::class_<RandomOperationMix>(m, "RandomOperationMix")
        .def(nb::init<const RandomOperationMix::OptionalRatioArray&>(),
             "ratios"_a)
        .def("__call__", &RandomOperationMix::operator(), "urng"_a)
        .def_ro("ratios", &RandomOperationMix::ratios);
    nb::class_<ExponentRange>(m, "ExponentRange")
        .def(nb::init<BitCount, BitCount>(), "min"_a, "max"_a)
        .def("min", &ExponentRange::min)
        .def("max", &ExponentRange::max);
    nb::class_<Range>(m, "Range")
        .def(nb::init<uint32_t, uint32_t>(), "min"_a, "max"_a)
        .def("min", &Range::min)
        .def("max", &Range::max)
        .def("is_power2", &Range::is_power2)
        .def("to_exponent_range", &Range::to_exponent_range);

    nb::bind_vector<std::vector<Address>>(m, "AddressVector");
    nb::bind_vector<std::vector<Operation>>(m, "Thread");
    nb::bind_vector<TraceInstance>(m, "TraceInstance");
    nb::bind_vector<std::vector<TraceInstance>>(m, "TraceInstanceVector");
    nb::bind_vector<std::vector<Event>>(m, "EventVector");
    nb::bind_vector<std::vector<Thread>>(m, "ThreadVector");
    nb::bind_map<TraceSelection>(m, "TraceSelection");

    nb::class_<Test>(m, "Test")
        .def(nb::init<const std::vector<Address>&,
                      const std::vector<Thread>&,
                      InstanceCount>(),
             "addresses"_a,
             "threads"_a,
             "instances"_a)
        .def_rw("addresses", &Test::addresses)
        .def_rw("threads", &Test::threads)
        .def_rw("instances", &Test::instances)
        .def("write", &Test::write, "filename"_a);

    nb::enum_<Monitor>(m, "Monitor")
        .value("LOCAL_COMMIT", Monitor::LOCAL_COMMIT)
        .value("REMOTE_COMMIT", Monitor::REMOTE_COMMIT)
        .value("LOCAL_COMPLETION", Monitor::LOCAL_COMPLETION)
        .value("REMOTE_COMPLETION", Monitor::REMOTE_COMPLETION)
        .value("SQUASH", Monitor::SQUASH)
        .value("AVAILABILITY", Monitor::AVAILABILITY)
        .value("MEMORY", Monitor::MEMORY)
        .value("ENTER_TIME", Monitor::ENTER_TIME)
        .value("COMMIT_TIME", Monitor::COMMIT_TIME)
        .value("NONE", Monitor::NONE)
        .export_values();

    nb::class_<Event>(m, "Event")
        .def(nb::init<Address,
                      Data,
                      Operation::Type,
                      Monitor,
                      ThreadID,
                      ProgramCounter,
                      Time>(),
             "address"_a,
             "data"_a,
             "type"_a,
             "source"_a,
             "tid"_a,
             "pc"_a,
             "time"_a)
        .def_rw("address", &Event::address)
        .def_rw("value", &Event::value)
        .def_rw("type", &Event::type)
        .def_rw("source", &Event::source)
        .def_rw("tid", &Event::tid)
        .def_rw("pc", &Event::pc)
        .def_rw("time", &Event::time);

    nb::class_<Trace>(m, "Trace")
        .def(nb::init<const std::vector<std::vector<Event>>&>(), "instances"_a)
        .def("filter", &Trace::filter, "selection"_a)
        .def("write", &Trace::write, "filename"_a)
        .def_static("read", &Trace::read, "filename"_a)
        .def("write_log",
             nb::overload_cast<const std::filesystem::path&, MemoryTraits>(
                 &Trace::write_log, nb::const_),
             "filename"_a,
             "memory_traits"_a = MemoryTraits{})
        .def_static("read_log", &Trace::read_log, "filename"_a)
        .def_rw("instances", &Trace::instances);

    nb::class_<TimeCounter>(m, "TimeCounter")
        .def(nb::init<>())
        .def("reset", &TimeCounter::reset)
        .def("start", &TimeCounter::start)
        .def("stop", &TimeCounter::stop)
        .def("milliseconds", &TimeCounter::milliseconds)
        .def("nanoseconds", &TimeCounter::nanoseconds)
        .def("seconds", &TimeCounter::seconds);

    auto checker = m.def_submodule("checker");
    nb::enum_<checker::Type>(checker, "Type")
        .value("HOCH", checker::Type::HOCH)
        .value("EVCH", checker::Type::EVCH)
        .value("MRSB", checker::Type::MRSB)
        .value("SRSB", checker::Type::SRSB)
        .value("LCHECK", checker::Type::LCHECK)
        .export_values();

    auto generator = m.def_submodule("generator");

    auto biasing =
        nb::class_<generator::Biasing>(generator, "Biasing")
            .def(nb::init<MemoryTraits, CacheTraits, bool, Seed>(),
                 "memory_traits"_a,
                 "cache_traits"_a,
                 "sbc"_a,
                 "seed"_a = std::random_device{}())
            .def("__call__",
                 nb::overload_cast<AddressCount,
                                   const generator::Biasing::CBC&,
                                   BitCount>(&generator::Biasing::operator()),
                 "s"_a,
                 "cbc"_a,
                 "abc"_a)
            .def("__call__",
                 nb::overload_cast<Range&>(&generator::Biasing::operator()),
                 "range"_a)
            .def("__call__",
                 nb::overload_cast<Range&, BitCount>(
                     &generator::Biasing::operator()),
                 "range"_a,
                 "abc"_a)
            .def("abc", &generator::Biasing::abc)
            .def("cbc", &generator::Biasing::cbc);
    nb::class_<generator::Biasing::CBC>(biasing, "CBC")
        .def(nb::init<AddressCount, AddressCount>(), "k"_a, "x"_a)
        .def_rw("k", &generator::Biasing::CBC::k)
        .def_rw("x", &generator::Biasing::CBC::x);

    auto chaining = nb::class_<generator::Chaining>(generator, "Chaining");
    nb::class_<generator::Chaining::Mix>(chaining, "Mix")
        .def(nb::init<const generator::Chaining::Mix::RatioArray&>(),
             "ratios"_a);
    nb::class_<generator::Chaining::RandomMix>(chaining, "RandomMix")
        .def(nb::init<
                 const generator::Chaining::RandomMix::OptionalRatioArray&>(),
             "ratios"_a)
        .def("__call__", &generator::Chaining::RandomMix::operator(), "urng"_a)
        .def_ro("ratios", &generator::Chaining::RandomMix::ratios);
    chaining
        .def(nb::init<ThreadCount,
                      const generator::Chaining::RandomMix&,
                      Seed>(),
             "thread_count"_a,
             "mix"_a = generator::Chaining::RandomMix{},
             "seed"_a = std::random_device{}())
        .def("__call__",
             nb::overload_cast<TestSize,
                               const generator::Chaining::Mix&,
                               AddressCount>(&generator::Chaining::operator()),
             "n"_a,
             "mix"_a,
             "s"_a)
        .def("__call__",
             nb::overload_cast<Range&, AddressCount>(
                 &generator::Chaining::operator()),
             "range"_a,
             "s"_a)
        .def("__call__",
             nb::overload_cast<TestSize, AddressCount>(
                 &generator::Chaining::operator()),
             "n"_a,
             "s"_a)
        .def("mix", &generator::Chaining::mix);

    nb::class_<generator::RandomAddress>(generator, "RandomAddress")
        .def(nb::init<MemoryTraits, Seed>(),
             "memory_traits"_a,
             "seed"_a = std::random_device{}())
        .def("__call__",
             nb::overload_cast<AddressCount>(
                 &generator::RandomAddress::operator()),
             "s"_a)
        .def("__call__",
             nb::overload_cast<Range&>(&generator::RandomAddress::operator()),
             "range"_a);
    nb::class_<generator::RandomThread>(generator, "RandomThread")
        .def(nb::init<ThreadCount, bool, const RandomOperationMix&, Seed>(),
             "thread_count"_a,
             "uniform_threads"_a = true,
             "mix"_a = RandomOperationMix{},
             "seed"_a = std::random_device{}())
        .def("__call__",
             nb::overload_cast<TestSize, const OperationMix&, AddressCount>(
                 &generator::RandomThread::operator()),
             "n"_a,
             "mix"_a,
             "s"_a)
        .def("__call__",
             nb::overload_cast<Range&, AddressCount>(
                 &generator::RandomThread::operator()),
             "range"_a,
             "s"_a)
        .def("__call__",
             nb::overload_cast<TestSize, AddressCount>(
                 &generator::RandomThread::operator()),
             "n"_a,
             "s"_a)
        .def("mix", &generator::RandomThread::mix);

    nb::class_<generator::HTG>(generator, "HTG")
        .def(nb::init<MemoryTraits,
                      CacheTraits,
                      ThreadCount,
                      coverage::Type,
                      ExponentRange,
                      ExponentRange,
                      BitCount,
                      bool,
                      InstanceCount,
                      const generator::Chaining::Mix&,
                      Seed>(),
             "memory_traits"_a,
             "cache_traits"_a,
             "thread_count"_a,
             "coverage_type"_a,
             "n_range"_a,
             "s_range"_a,
             "abc"_a,
             "sbc"_a,
             "instances"_a,
             "mix"_a = generator::HTG::default_mix,
             "seed"_a = std::random_device{}())
        .def("__call__", &generator::HTG::operator())
        .def("feedback", &generator::HTG::feedback, "output"_a)
        .def("constraints", &generator::HTG::constraints);

    auto sca = nb::class_<generator::SCA>(generator, "SCA")
                   .def(nb::init<generator::SCA::Variant,
                                 MemoryTraits,
                                 CacheTraits,
                                 ThreadCount,
                                 ExponentRange,
                                 ExponentRange,
                                 BitCount,
                                 bool,
                                 InstanceCount,
                                 const generator::Chaining::Mix&,
                                 Seed>(),
                        "variant"_a,
                        "memory_traits"_a,
                        "cache_traits"_a,
                        "thread_count"_a,
                        "n_range"_a,
                        "s_range"_a,
                        "abc"_a,
                        "sbc"_a,
                        "instances"_a,
                        "mix"_a = generator::SCA::default_mix,
                        "seed"_a = std::random_device{}())
                   .def("__call__", &generator::SCA::operator())
                   .def("feedback", &generator::SCA::feedback, "output"_a)
                   .def("constraints", &generator::SCA::constraints);
    nb::enum_<generator::SCA::Variant>(sca, "Variant")
        .value("COVERAGE", generator::SCA::Variant::COVERAGE)
        .value("TRADEOFF", generator::SCA::Variant::TRADEOFF)
        .value("EFFORT", generator::SCA::Variant::EFFORT)
        .export_values();

    auto mcversi = nb::class_<generator::McVerSi>(generator, "McVerSi");
    nb::class_<generator::mcversi::OperationMix>(mcversi, "OperationMix")
        .def(nb::init<const generator::mcversi::OperationMix::RatioArray&>(),
             "ratios"_a)
        .def_ro("ratios", &generator::mcversi::OperationMix::ratios);
    mcversi
        .def(nb::init<ThreadCount,
                      ByteCount,
                      ByteCount,
                      InstanceCount,
                      TestSize,
                      size_t,
                      size_t,
                      size_t,
                      float,
                      double,
                      double,
                      ByteCount,
                      ByteCount,
                      TestSize,
                      const generator::mcversi::OperationMix&,
                      Seed>(),
             "thread_count"_a,
             "memory_size"_a,
             "stride"_a,
             "instances"_a,
             "test_size"_a,
             "population_size"_a = generator::McVerSi::default_population_size,
             "tournament_size"_a = generator::McVerSi::default_tournament_size,
             "mates"_a = generator::McVerSi::default_mates,
             "mutation_rate"_a = generator::McVerSi::default_mutation_rate,
             "p_usel"_a = generator::McVerSi::default_p_usel,
             "p_bfa"_a = generator::McVerSi::default_p_bfa,
             "chunk_size"_a = generator::McVerSi::default_chunk_size,
             "hole_size"_a = generator::McVerSi::default_hole_size,
             "nop_max_seq_size"_a =
                 generator::McVerSi::default_nop_max_seq_size,
             "mix"_a = generator::McVerSi::default_mix,
             "seed"_a = std::random_device{}())
        .def("__call__", &generator::McVerSi::operator())
        .def("feedback", &generator::McVerSi::feedback, "output"_a)
        .def("nd_t", &generator::McVerSi::nd_t)
        .def("fitaddrs_count", &generator::McVerSi::fitaddrs_count)
        .def("test_info", &generator::McVerSi::test_info);

    auto coverage = m.def_submodule("coverage");

    nb::enum_<coverage::Type>(coverage, "Type")
        .value("FSM_STRUCTURAL", coverage::Type::FSM_STRUCTURAL)
        .value("FSM_PER_INSTANCE", coverage::Type::FSM_PER_INSTANCE)
        .export_values();

    nb::class_<coverage::Metric>(coverage, "Metric")
        .def(nb::init<coverage::Type, size_t>(), "type"_a, "max"_a = 1)
        .def("clone", &coverage::Metric::clone)
        .def("affects_value", &coverage::Metric::affects_value, "other"_a)
        .def("merge", &coverage::Metric::merge, "other"_a)
        .def("remove", &coverage::Metric::remove, "other"_a)
        .def("reset", &coverage::Metric::reset)
        .def("full", &coverage::Metric::full)
        .def("max", &coverage::Metric::max)
        .def("percentage", &coverage::Metric::percentage)
        .def("value", &coverage::Metric::value)
        .def("type", &coverage::Metric::type)
        .def("write", &coverage::Metric::write, "filename"_a)
        .def("write_csv", &coverage::Metric::write_csv, "filename"_a)
        .def_static("read",
                    nb::overload_cast<const std::filesystem::path&>(
                        &coverage::Metric::read),
                    "filename"_a);

    auto platform = m.def_submodule("platform");
    platform.def("signal_continue",
                 &platform::signal_continue,
                 "pid"_a,
                 "instance_count"_a);
    nb::class_<platform::Traits>(platform, "Traits")
        .def_ro_static("platform_dir", &platform::Traits::platform_dir)
        .def_ro_static("input_dir", &platform::Traits::input_dir)
        .def_ro_static("output_dir", &platform::Traits::output_dir)
        .def_ro_static("output_file", &platform::Traits::output_file)
        .def_ro_static("test_file", &platform::Traits::test_file)
        .def_static("makedirs", &platform::Traits::makedirs);
    auto output = nb::class_<platform::Output>(platform, "Output");
    nb::class_<platform::Output::Checker>(output, "Checker")
        .def(nb::init<checker::Type, checker::ErrorCode, Time>(),
             "type"_a,
             "code"_a,
             "time"_a)
        .def_rw("type", &platform::Output::Checker::type)
        .def_rw("code", &platform::Output::Checker::code)
        .def_rw("time", &platform::Output::Checker::time);
    nb::bind_vector<std::vector<platform::Output::Checker>>(output,
                                                            "CheckerVector");
    nb::bind_vector<std::vector<coverage::Metric>>(output, "MetricVector");
    output
        .def(nb::init<Time,
                      Time,
                      const std::vector<platform::Output::Checker>&,
                      const std::vector<coverage::Metric>&,
                      const std::vector<coverage::Metric>&,
                      Trace>(),
             "test_time"_a,
             "warmup_time"_a,
             "checkers"_a,
             "metrics"_a,
             "aggregated_metrics"_a,
             "trace"_a)
        .def_rw("test_time", &platform::Output::test_time)
        .def_rw("warmup_time", &platform::Output::warmup_time)
        .def_rw("checkers", &platform::Output::checkers)
        .def_rw("metrics", &platform::Output::metrics)
        .def_rw("aggregated_metrics", &platform::Output::aggregated_metrics)
        .def_rw("trace", &platform::Output::trace)
        .def("write", &platform::Output::write, "filename"_a)
        .def_static("read", &platform::Output::read, "filename"_a);
}
