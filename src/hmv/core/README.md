# HOME-V core

All core types that are shared between different modules of HOME-V are defined inside this directory.
Some of them are simple aliases to fundamental types such as integers, so not much explanation about them is required.
Just keep in mind that you should always use an alias instead of a fundamental type when applicable.

## Overview of core types

The sections below will describe all non-trivial core types and presents small snippets showcasing python bindings when available.

### `WordSize`

Header: [`hmv/core/types.hpp`](types.hpp)
Enumeration defining all supported word sizes:
* `DWORD`: 4 bytes words.
* `QWORD`: 8 bytes words.

#### Python bindings showcase
```python
>>> from hmv import WordSize
>>> WordSize(0)
WordSize.DWORD
>>> WordSize.DWORD.name
'DWORD'
>>> Monitor["DWORD"]
WordSize.DWORD
```

### `Atomicity`

Header: [`hmv/core/types.hpp`](types.hpp)
Enumeration defining all supported store atomicity types:
* `OMCA`: other-multi-copy atomic stores (adopted by most ISAs).
* `NMCA`: non-multi-copy atomic stores (uncommon).

### `MCM`

Header: [`hmv/core/types.hpp`](types.hpp)
Enumeration defining all supported Memory Consistency Models (MCM):
* `RELAXED`: relaxed memory consistency model.
* `TSO`: Total Store Order model.

### `ISA`

Header: [`hmv/core/types.hpp`](types.hpp)
Enumeration defining all supported Instruction Set Architectures (ISA):
* `ARM`: 64-bit ARM ISA.
* `RISCV`: 64-bit RISC-V ISA.

### `ISATraits`

Header: [`hmv/core/types.hpp`](types.hpp)
Plain struct defining properties of the Instruction Architecture Set (ISA). Contains the following fields:
* `architecture`: instruction set architecture (see [`ISA`](#isa)).
* `memory_model`: memory consistency model adopted by the ISA (see [`MCM`](#mcm)).
* `atomicity`: store atomicity adopted by the ISA (see [`Atomicity`](#atomicity)).

It is mostly used by checkers and the `hmv::platform::Controller`.

### `MemoryTraits`

Header: [`hmv/core/types.hpp`](types.hpp)
Plain struct defining properties of the shared memory space. Contains the following fields:
* `address_size`: address size (in bits) for allocating the test's shared memory (not the same as physical/virtual address size).
* `word_size`: word size (see [`WordSize`](#wordsize)).

You can think of `address_size` as defining a second virtual address space which is used by the generator and the test.
All mapping from this second virtual address space to the actual virtual address space and the physical address space are automatically handled by HOME-V.

#### Python bindings showcase
```python
>>> from hmv import MemoryTraits
>>> traits = MemoryTraits(32, 8)
>>> traits.address_size
32
>>> traits.word_size
8
```

### `CacheTraits`

Header: [`hmv/core/types.hpp`](types.hpp)
Plain struct defining cache properties. Contains the following fields:
* `size`: cache size (in bytes).
* `block_size`: cache block size (in bytes).
* `associativity`: cache associativity (1 for 1-way, 2 for 2-way, 4 for 4-way, etc).

It also provides two methods:
* `index_size`: calculates and returns the index size (in bits).
* `offset_size` calculates and returns the offset size (in bits).

#### Python bindings showcase
```python
>>> from hmv import CacheTraits
>>> traits = CacheTraits(4096, 64, 1)
>>> traits.size
4096
>>> traits.block_size
64
>>> traits.associativity
1
>>> traits.index_size()
6
>>> traits.offset_size()
6
```
### `Operation::Type`

Header: [`hmv/core/operation.hpp`](operation.hpp)
Enumeration defining all supported types of operations:
* `LOAD`: load operation.
* `STORE`: store operation.
* `FENCE`: memory fence operation.
* `NOP`: no operation.
* `ADD`: add operation between two registers.
* `XOR`: exclusive or between two registers.
* `NONE`: special value reserved for algorithmic purposes.

#### Python bindings showcase

```python
>>> from hmv import Operation
>>> Operation.Type(0)
Operation.Type.LOAD
>>> Operation.Type.LOAD.name
'LOAD'
>>> Operation.Type["LOAD"]
Operation.Type.LOAD
```

### `Register::Name`

Header: [`hmv/core/register.hpp`](register.hpp)
Enumeration defining all supported generic register names:
* `R1`: Maps to `x1` for ARMv8 and `a1` for RISC-V.
* `R2`: Maps to `x2` for ARMv8 and `a2` for RISC-V.
* `R3`: Maps to `x3` for ARMv8 and `a3` for RISC-V.
* `R4`: Maps to `x4` for ARMv8 and `a4` for RISC-V.
* `R5`: Maps to `x11` for ARMv8 and `t1` for RISC-V.
* `R6`: Maps to `x12` for ARMv8 and `t2` for RISC-V.
* `R7`: Maps to `x13` for ARMv8 and `t3` for RISC-V.
* `R8`: Maps to `x14` for ARMv8 and `t4` for RISC-V.
* `NONE`: Maps to a random register (from one of the above) when the corresponding field is required by an Operation.

#### Python bindings showcase

```python
>>> from hmv import Register
>>> Register.Name(0)
Name.R1
>>> Register.Name.R1.name
'R1'
>>> Register.Name["R1"]
Name.R1
```

### `Register::Usage`

Header: [`hmv/core/register.hpp`](register.hpp)
Enumeration defining all usage types for registers:
* `DEFAULT`: Default usage as defined by each operation.
* `ASSIGN_ADDRESS`: Assigns the target address to the associated register.
* `ASSIGN_VALUE`: Assigns the operation value to the associated register.
* `PRESERVE`: Don't assign anything to the associated register, use it as-is.

#### Python bindings showcase

```python
>>> from hmv import Register
>>> Register.Usage(0)
Usage.DEFAULT
>>> Register.Usage.DEFAULT.name
'DEFAULT'
>>> Register.Usage["DEFAULT"]
Usage.DEFAULT
```

### `Register`

Header: [`hmv/core/register.hpp`](register.hpp)
Class encapsulating a `Register::Name` and `Register::Usage` into a single `uint8_t` value.
It provides the following methods:
* `Name`: operator that returns the `Register::Name` value.
* `usage`: getter/setter methods to get/change the `Register::Usage` value.

#### Python bindings showcase

```python
>>> from hmv import Register
>>> register = Register(Register.Name.R5, Register.Usage.PRESERVE)
```

### `Operation`

Header: [`hmv/core/operation.hpp`](operation.hpp)
Plain struct defining an operation. Contains the following fields:
* `address`:  index of the target address in the test's addresses list.
* `value`: value to write if applicable.
* `type`: type of the operation (see [`Operation::Type`](#operation::type)).
* `dest`: register to use on the destination field (see [`Register`](#Register)).
* `src1`: register to use on the first source field (see [`Register`](#Register)).
* `src2`: register to use on the second source field (see [`Register`](#Register)).
Specializations for each type of operation are provided to facilitate instantiating them with less verbosity:
* `Load`: specialization of `Operation` constraining `type` to `hmv::Operation::Type::LOAD`.
* `Store`: specialization of `Operation` constraining `type` to `hmv::Operation::Type::STORE`.
* `Fence`: specialization of `Operation` constraining `type` to `hmv::Operation::Type::FENCE`.
* `Nop`: specialization of `Operation` constraining `type` to `hmv::Operation::Type::NOP`.
* `Add`: specialization of `Operation` constraining `type` to `hmv::Operation::Type::ADD`.
* `Xor`: specialization of `Operation` constraining `type` to `hmv::Operation::Type::XOR`.

Not only they constrain the `type` field, but also default-initialize unused fields.
For instance, `Load` only requires a `address`, since the `value` field has no meaning for it.

There are a few operators and helper methods to check the type of the operation.
They are self-explanatory, just keep in mind they're not available in the python bindings (if needed, they can be added).

#### Python bindings showcase

```python
from hmv import Load, Store, Fence
>>> fence = Fence()
>>> fence.type
Operation.Type.FENCE
>>> load = Load(0)
>>> load.type
Operation.Type.LOAD
>>> load.address
0
>>> store = Store(1, 5678)
>>> store.type
Operation.Type.STORE
>>> store.address
1
>>> store.value
5678
>>>
```

### `Monitor`

Header: [`hmv/core/event.hpp`](event.hpp)
Enumeration defining all monitors supported:
* `LOCAL_COMMIT` - read/write/fence events capturing the exit of the commit unit.
* `REMOTE_COMMIT` - write events capturing the entry of the incoming L1 cache buffers.
* `LOCAL_COMPLETION` - read events capturing the reply to the read request (either coming from the L1 cache or forwarded from the store queue) and write events capturing the exit of the store queue (after the write took effect).
* `REMOTE_COMPLETION` - write events capturing the exit of incoming L1 cache buffers.
* `SQUASH` - read/write events capturing the squash of operations at the load/store queues.
* `AVAILABILITY` - write events capturing when the value of a store is written to the store queue.
* `MEMORY` - equivalent to `LOCAL_COMPLETION`, excluding replies forwarded from the store queue.
* `ENTER_TIME` - read/write/fence events capturing the entry of the fetch unit.
* `COMMIT_TIME` - equivalent to `LOCAL_COMMIT` for reads and fences, equivalent to `LOCAL_COMPLETION` for writes.
* `NONE` - special value used when an [`Event`](#event)/[`Operation`](#operation) is not sourced from a monitor.

#### Python bindings showcase
```python
>>> from hmv import Monitor
>>> Monitor(0)
Monitor.LOCAL_COMMIT
>>> Monitor.LOCAL_COMMIT.name
'LOCAL_COMMIT'
>>> Monitor["LOCAL_COMMIT"]
Monitor.LOCAL_COMMIT
```

### `Event`

Header: [`hmv/core/event.hpp`](event.hpp)
Plain struct inheriting from `Operation`. It adds the following fields:
* `source`: monitor that is the source of the event (see [`Monitor`](#monitor)).
* `tid`: thread ID from which the event originated.
* `pc`: program counter of the associated operation (not all monitors are capable of capturing this information, in which case it will be set to 0).
* `time`: time (in cycles) in which the event happened (relative to a global clock, can be used to infer time order between different core domains).

It is an extended version of the notion of memory events defined in [Speck&Check](https://doi.org/10.1109/ICCAD45719.2019.8942040).
Some fields were added due to particular requirements from specific checkers (for instance, `time` is a requirement of `LCHECK`).

There are a few additional specializations of `Event` that are defined to statically differentiate types of events.
Similar to `Operation` specializations, they simplify the constructors to only require relevant fields.
They can be divided in two groups.

The first group annotates the type of operation, similar to the `Operation` specializations:
* `ReadEvent`: specialization of `Event` constraining `type` to `hmv::Operation::Type::LOAD`.
* `WriteEvent`: specialization of `Event` constraining `type` to `hmv::Operation::Type::STORE`.
* `FenceEvent`: specialization of `Event` constraining `type` to `hmv::Operation::Type::FENCE`.

The second group annotates the both the type of operation and the source `Monitor`:
* `ReadAt<monitor>`: specialization of `ReadEvent`, constraining `source` to `monitor` (where `monitor` is a value of `hmv::Monitor`).
* `WriteAt<monitor>`: specialization of `WriteEvent`, constraining `source` to `monitor` (where `monitor` is a value of `hmv::Monitor`).
* `FenceAt<monitor>`: specialization of `FenceEvent`, constraining `source` to `monitor` (where `monitor` is a value of `hmv::Monitor`).

These specializations allow checkers to statically define (in the signature of the `push` methods) exactly which types of events they need to receive (see the guide on [checkers](../checker/README.md)).
There are no specializations related to RAW hazards, since they are annotations for the test generation and don't reflect real operations that can be monitored as events.

### `Test`

Header: [`hmv/core/test.hpp`](test.hpp)
Plain struct defining a test. Contains the following fields:
* `addresses`: vector of all addresses used by the test (contained within the spaced defined by `address_size` of `MemoryTraits`).
* `threads`: vector of vectors of operations. Each operation should use `address` as the index to the actual address from the `addresses` vector.
* `instances`: number of execution instances to run this test. Each instance is a repetition of the entire test.

This struct is one of the most important data types for generators (see the guide on [generators](../generator/README.md)).
It allows to completely abstract ISA details from the generators, creating a portable representation of tests.

#### Python bindings showcase
```python
>>> from hmv import Load, Store, Fence, Test
>>> addresses = [1234, 5678, 9012]
>>> threads = [[Load(0), Store(1, 1), Load(2)], [Store(0, 2), Load(1), Store(2, 3)]]
>>> test = Test(addresses, threads, 10)
>>> test.addresses
hmv._hmv_internal.AddressVector([1234, 5678, 9012])
>>> test.threads
hmv._hmv_internal.ThreadVector([hmv._hmv_internal.Thread([<hmv._hmv_internal.Operation object at 0x7f77feb0ab20>, <hmv._hmv_internal.Operation object at 0x7f77feb0ab20>, <hmv._hmv_internal.Operation object at 0x7f77feb0ab20>]), hmv._hmv_internal.Thread([<hmv._hmv_internal.Operation object at 0x7f77feb0ab20>, <hmv._hmv_internal.Operation object at 0x7f77feb0ab20>, <hmv._hmv_internal.Operation object at 0x7f77feb0ab20>])])
>>> test.instances
10
```

### `Trace`

Header: [`hmv/core/trace.hpp`](trace.hpp)
Plain struct defining a trace of events. Contains the following field:
* `instances`: vector of vectors of events, each vector corresponding to the events of an execution instance of the test.

It also provides the following methods:
* `serialize`: serializes the trace to a binary output file. Should not be called directly, use `write` instead.
* `write`: writes the trace to the specified filename, creating a binary file and calling `serialize` internally.
* `deserialize`: deserializes the trace from a binary input file. Should not be called directly, use `read` instead.
* `read`: reads the trace from the specified filename, opening it in binary mode and calling `deserialize` internally.
* `write_log`: writes the trace to the specified filename, serializing it in a human-readable format.
* `read_log`: reads the trace from the specified filename, deserializing it from a human-readable format.

This struct is essential for generators that require event information from the execution of a test as part of their feedback (see the guide on [generators](../generator/README.md)).
Generators can use the `TraceSelection` (an alias to `std::unordered_map<Operation::Type, std::set<Monitor>>`) to select which operations and from which monitors they require a trace.

#### Python bindings showcase
```python
>>> from hmv import Trace, TraceSelection, Operation, Monitor
>>> trace = Trace.read_log("some.log")
>>> len(trace.instances)
100
>>> trace.instances[0][0].type
Operation.Type.LOAD
>>> trace.instances[0][0].source
Monitor.LOCAL_COMMIT
>>> trace.write("binary_trace.bin")
>>> trace2 = Trace.read("binary_trace.bin")
>>> len(trace.instances)
100
>>> trace2.instances[0][0].type
Operation.Type.LOAD
>>> trace2.instances[0][0].source
Monitor.LOCAL_COMMIT
>>> selection = TraceSelection({Operation.Type.LOAD: {Monitor.LOCAL_COMMIT}, Operation.Type.STORE: {Monitor.LOCAL_COMPLETION}})
>>> selection[Operation.Type.LOAD]
{Monitor.LOCAL_COMMIT}
```

### `TimeCounter`

Header: [`hmv/core/time_counter.hpp`](time_counter.hpp)
Class defining the standard way of measuring time inside HOME-V. Provides the following methods:
* `reset`: resets the time counter to 0.
* `start`: starts counting time.
* `stop`: stops counting time.
* `milliseconds`: returns the current (integer) amount of time counted in milliseconds (can be used without calling `stop` first).
* `nanoseconds`: returns the current (integer) amount of time counted in nanoseconds (can be used without calling `stop` first).
* `seconds`: returns the current (float) amount of time counted in seconds (can be used without calling `stop` first).

#### Python bindings showcase
```python
>>> counter = TimeCounter()
>>> counter.nanoseconds()
0
>>> counter.start()
>>> counter.nanoseconds()
5697775015
>>> counter.stop()
>>> counter.nanoseconds()
10395719513
>>> counter.reset()
>>> counter.nanoseconds()
0
```
