# HOME-V checkers

All checkers supported by HOME-V are implemented inside this directory.
The following sections will guide you on how to add a new checker and how to add a new monitor.

## How to add a new checker

Each checker has to provide a single top level entity to interface with.
If your checker is split into several objects (e.g. one for each thread), aggregate all of them behind a top level class.
By default, HOME-V will track your checker's execution time and generate a trace of all events in case of error detection (or if `-d/--debug` flag is provided to the platform), so you don't need to implement those features yourself.

Your new checker should have a single header/source pair inside `src/hmv/checker` containing the top level class.
If you want to have more header/source files, you can create a `src/hmv/checker/component/MY_CHECKER/` subdirectory to store them.
If you're defining reusable components shared between multiple checkers, you may put them directly inside `src/hmv/checker/component`.
All checker related code and classes should be declared inside the `hmv::checker` namespace.

### Required includes

Two includes will always be required, regardless of how your checker operates under the hood:
```cpp
#include "component/report.hpp"
#include "hmv/core/event.hpp"
```

The first one provides the definition of a struct that is fundamental in one of the required methods that will be presented later.
The second one provides the definition of memory events, another fundamental type for checkers.

Additionally, you may explicitly include the header with all basic type definitions, since you will most likely be using them:
```cpp
#include "hmv/core/types.hpp"
```

### Mandatory methods

There are three mandatory methods and one enumeration that you need to implement inside the top level class of your new checker.

#### Constructor

Always declare a constructor receiving a `ThreadCount` and [`ISATraits`](../core/README.md#isatraits) (even if your checker doesn't require any information):
```cpp
namespace hmv::checker {
// ...
    class MyNewChecker {
    // ...
     public:
        MyNewChecker(ThreadCount thread_count, ISATraits traits);
    // ...
    };
// ...
} // namespace hmv::checker
```

#### `push`

This method is used to check memory events (return true if an error is detected, false otherwise).
By simply defining methods with the `push` signature, you select which events your checker will receive (only events that are supported by the checker are forwarded).
For instance, if you want to receive all types of events without discerning them:
```cpp
bool MyNewChecker::push(const Event& event) {
    // checker implementation for events go here
    // ...
    return false; // no error
}
```

Or any events at a specific monitor (e.g. LOCAL_COMMIT):
```cpp
template <template <Monitor> typename EventAt>
bool MyNewChecker::push(const EventAt<LOCAL_COMMIT>& event) {
    // checker implementation for local commit events go here
    // ...
    return false; // no error
}
```

Or only write events at a specific monitor (e.g. AVAILABILITY):
```cpp
bool MyNewChecker::push(const WriteAt<AVAILABILITY>& write) {
    // checker implementation for local write availability events go here
    // ...
    return false; // no error
}
```

You get the idea. All `Event` specializations are described [here](../core/README.md#event).
Additionally, you can use something like `template <template <Monitor> typename EventAt>` from the previous examples to accept any events at a specific monitor.

#### `report` and `Error` enumeration

The `report` method usually consists of a simple getter to return a `Report`:
```cpp
const Report& MyNewChecker::report() const {
    return error_report;
}
```

The usual flow is to create the report when an error is detected and save it in a member field so the platform can retrieve it later with the `report` method.

The `Report` struct is defined in [`src/hmv/checker/component/report.hpp`](component/report.hpp), and contains the following fields:
* `code`: an unsigned value representing the error code of your checker (defined in an enum, more on it below).
* `description`: a string describing the result (e.g. what is the error).
* `events`: a vector of "offending" events that lead to the result.

In the same file, a function called `make_report` and a helper struct called `ReportStatusInfo` are defined to facilitate the process of creating reports.
To use them, first you need to define an `Error` enumeration with all the errors your checker can raise (declared inside the top level class):
```cpp
namespace hmv::checker {
// ...
    class MyNewChecker {
    // ...
     public:
        // Underlying type set to match Report::code field
        enum Error : ErrorCode {
            NONE = 0, // Always define NONE
            ERROR_1 = 1,
            ERROR_2 = 2,
            // ...
            ERROR_N = 255,
        };
        // ...
     private:
        // ...
        Report error_report;
    // ...
    };
} // namespace hmv::checker
```

And define full template specializations of `ReportStatusInfo` for each of your errors:
```cpp
namespace hmv::checker {
    template <>
    struct ErrorInfo<MyNewChecker::Error::NONE> {
        static constexpr auto description = "no error found";
    };

    template <>
    struct ErrorInfo<MyNewChecker::Error::ERROR_1> {
        static constexpr auto description = "description of ERROR_1";
    };

    template <>
    struct ErrorInfo<MyNewChecker::Error::ERROR_2> {
        static constexpr auto description = "description of ERROR_2";
    };
    // ...

    template <>
    struct ErrorInfo<MyNewChecker::Error::ERROR_N> {
        static constexpr auto description = "description of ERROR_3";
    };
} // namespace hmv
```

Now you can use `make_report` to produce a simple report with `result` and `description` populated directly from your `ErrorInfo` specializations:
```cpp
// ...
error_report = make_report<Error::ERROR_1>();
// ...
```

And to specify "offending" events:
```cpp
// ...
error_report = make_report<Error::ERROR_2>(event_a, event_b);
// ...
```

Or passing events from an existing container:
```cpp
// ...
std::vector<Event> events;
// ...
error_report = make_report<Error::ERROR_N>(events);
// ...
```

### Optional methods

There are 7 optional methods that can be implemented, indicating specific phases of the execution of a test.

#### `begin_test`

Called when a test begins.
It receives the test that will be executed.
Full signature:
```cpp
void MyNewChecker::begin_test(const Test& test);
```

#### `begin_instance`

Called when an execution instance begins.
Precedes all `begin_thread` calls.
Full signature:
```cpp
void MyNewChecker::begin_instance();
```

#### `begin_thread`

Called when a thread begins its execution of an execution instance.
It receives the `ThreadID` of the thread.
Full signature:
```cpp
void MyNewChecker::begin_thread(ThreadID tid);
```

#### `end_thread`

Called when a thread ends its execution of an execution instance.
It receives the `ThreadID` of the thread and can be used to raise an error detection (return true if an error is detected, false otherwise).
Full signature:
```cpp
bool MyNewChecker::end_thread(ThreadID tid);
```

#### `end_instance`

Called when an execution instance ends.
Succeeds all `end_thread` calls and can be used to raise an error detection (return true if an error is detected, false otherwise).
Full signature:
```cpp
bool MyNewChecker::end_instance();
```

#### `end_test`

Called when a test ends.
It can be used to raise an error detection (return true if an error is detected, false otherwise).
Post-mortem checkers should use this function to perform their algorithms.
Full signature:
```cpp
bool MyNewChecker::end_test();
```

#### `write`

Called only if a trace is generated (due to error detection or debug flag).
It receives the path of the output directory and can be used to write additional information in a checker-specific file.
Full signature:
```cpp
void MyNewChecker::write(const std::filesystem::path& output_dir);
```

### Platform-independent changes

Add your new checker to the `hmv::checker::Type` enumeration in [`src/hmv/checker/checker.hpp`](checker.hpp):
```cpp
    enum class Type : uint8_t {
        HOCH = 0,
        EVCH = 1,
        // ...
        MY_NEW_CHECKER = 5,
    };
```

And add a case statement to instantiate your new checker and to convert the Type to string in [`src/hmv/checker/checker.cpp`](checker.cpp):
```cpp
Checker::Checker(Type type,
                 MemoryTraits memory_traits,
                 ThreadCount thread_count,
                 ISATraits traits,
                 const std::filesystem::path& log_dir,
                 bool enable_logger)
    : tag{type} {
    switch (type) {
    case Type::HOCH:
        model = std::make_unique<Model<HOCH>>(memory_traits,
                                              thread_count,
                                              traits,
                                              log_dir / to_string(tag),
                                              enable_logger);
        break;
    case Type::EVCH:
        model = std::make_unique<Model<EVCH>>(memory_traits,
                                              thread_count,
                                              traits,
                                              log_dir / to_string(tag),
                                              enable_logger);
        break;
    // ...
    case Type::MY_NEW_CHECKER:
        model = std::make_unique<Model<MY_NEW_CHECKER>>(memory_traits,
                                                        thread_count,
                                                        traits,
                                                        log_dir / to_string(tag),
                                                        enable_logger);
        break;
    }
    assert(model);
}

std::string hmv::checker::to_string(Type type) {
    switch (type) {
    case Type::HOCH:
        return "HOCH";
    case Type::EVCH:
    // ...
    case Type::MY_NEW_CHECKER:
        return "MY_NEW_CHECKER"; // must be equal to the name used in Type enum
    }
    __builtin_unreachable();
}
```

Finally, update the python binding in [`src/hmv/bindings/nanobind.cpp`](../bindings/nanobind.cpp):
```cpp
    nb::enum_<checker::Type>(checker, "Type")
        .value("HOCH", checker::Type::HOCH)
        .value("EVCH", checker::Type::EVCH)
        // ...
        .value("MY_NEW_CHECKER", checker::Type::MY_NEW_CHECKER)
        .export_values();
```

### Platform-specific changes

Some platform-specific changes are required to make your checker available in them.

#### gem5 integration

Add your new checker to the `HMVChecker` enumeration in [`src/hmv/integration/gem5/HMVController.py`](../integration/gem5/HMVController.py):
```python
class HMVChecker(ScopedEnum):
    # Keep this list in sync with hmv::checker::Type
    vals = ["HOCH", "EVCH", ..., "MY_NEW_CHECKER"]
```

## How to add a new monitor

### Platform-independent changes

First, add your new monitor to the `Monitor` enumeration in [`src/hmv/core/event.hpp`](../core/event.hpp):
```cpp
    enum Monitor : uint8_t {
        LOCAL_COMMIT = 0,
        // ...
        MY_NEW_MONITOR = 9,
        NONE = 255,
    };
```

Also update the python binding to include your new monitor in [`src/hmv/bindings/nanobind.cpp`](../bindings/nanobind.cpp):
```cpp
    nb::enum_<Monitor>(m, "Monitor")
        .value("LOCAL_COMMIT", Monitor::LOCAL_COMMIT)
        // ...
        .value("MY_NEW_MONITOR", Monitor::MY_NEW_MONITOR)
        .value("NONE", Monitor::NONE)
        .export_values();

```

Add to/from string conversions in [`src/hmv/core/event.cpp`](../core/event.cpp):
```cpp
Monitor hmv::to_monitor(const std::string& monitor) {
    switch (monitor.size()) {
    case 6:
        if (monitor == "MEMORY") {
            return Monitor::MEMORY;
        } else if (monitor == "SQUASH") {
            return Monitor::SQUASH;
        }
        break;
    // ...
    case 14: // make sure to use the case with the correct string size
        if (monitor == "MY_NEW_MONITOR") {
            return Monitor::MY_NEW_MONITOR;
        }
        break;
    // ...
    }
    return Monitor::NONE;
}

std::string hmv::to_string(Monitor monitor) {
    switch (monitor) {
    case Monitor::LOCAL_COMMIT:
        return "LOCAL_COMMIT";
    // ...
    case Monitor::MY_NEW_MONITOR:
        return "MY_NEW_MONITOR";
    case Monitor::NONE:
        return "NONE";
    }
    __builtin_unreachable();
}
```

And finally, add virtual methods to `Checker::Concept` and `Checker::Model` classes with the valid combinations of your monitor and types of events.
For the rest of the code snippets, we will assume the new monitor is only valid for read and write events.
Add the following boilerplate in [`src/hmv/checker/checker.hpp`](checker.hpp):
```cpp
namespace hmv::checker {
    // ...
    class Checker {
        // ...
        class Concept {
         public:
            // ...
            virtual bool push(const ReadAt<MY_NEW_MONITOR>& event) = 0;
            virtual bool push(const WriteAt<MY_NEW_MONITOR>& event) = 0;
            // ...
        };
        // ...
        template <typename C>
        class Model final : public Concept {
         public:
            // ...
            bool push(const ReadAt<MY_NEW_MONITOR>& event) override;
            bool push(const WriteAt<MY_NEW_MONITOR>& event) override;
            // ...
        };
        // ...
    };
    // ...
    template <typename C>
    inline bool Checker::Model<C>::push(const ReadAt<MY_NEW_MONITOR>& event) {
        return try_push(event);
    }

    template <typename C>
    inline bool Checker::Model<C>::push(const WriteAt<MY_NEW_MONITOR>& event) {
        return try_push(event);
    }
    // ...
} // namespace hmv::checker
```

### Platform-specific changes

#### gem5 integration

First, update the gem5 script to allow tracing of your new monitor.
Add the following in [`src/hmv/integration/gem5/configs/hmv.py](../integration/gem5/configs/hmv.py):
```python
_trace_flags = {
    "LOCAL_COMMIT": [0],
    # ...
    "MY_NEW_MONITOR": [9], # map the name of your monitor to the enum value
    "ALL": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9], # remember to add the value here too
}
```

Next, add boilerplate specializations of the `gem5::HMVController::push_event` method in [`src/hmv/integration/gem5/hmv_controller.cc`](../integration/gem5/hmv_controller.cc):
```cpp
template <>
void HMVController::push_event(const ReadAt<Monitor::MY_NEW_MONITOR>& event) {
    core->push_event(event, end_simulation);
}

template <>
void HMVController::push_event(const WriteAt<Monitor::MY_NEW_MONITOR>& event) {
    core->push_event(event, end_simulation);
}
```

Create new method(s) in `gem5::HMVThreadMonitor` to generate the actual events of your monitor.
Add the following in [`src/hmv/integration/gem5/hmv_thread_monitor.hh`](../integration/gem5/hmv_thread_monitor.hh):
```cpp
namespace gem5 {
    // ...
    class HMVThreadMonitor : public SimObject {
     public:
        // ...
        void my_new_monitor(const o3::DynInstPtr& inst);
        // or, if you collect reads and writes at different spots:
        void my_new_monitor_read(const o3::DynInstPtr& inst);
        void my_new_monitor_write(const o3::DynInstPtr& inst);
        // ...
    };
    // ...
} // namespace gem5
```

And the following in [`src/hmv/integration/gem5/hmv_thread_monitor.cc`](../integration/gem5/hmv_thread_monitor.cc):
```cpp
void HMVThreadMonitor::my_new_monitor(const o3::DynInstPtr& inst) {
    if (!started) {
        return;
    }
    auto time = controller.current_time();
    if (auto address = controller.match_address(inst->physEffAddr)) {
        auto pc = normalize_pc(inst);
        if (inst->isLoad()) {
            auto value = get_value(inst);
            controller.push_event(ReadAt<Monitor::MY_NEW_MONITOR>{
                address.value(), value, tid, pc, time});
        } else if (inst->isStore()) {
            auto value = get_value(inst);
            controller.push_event(WriteAt<Monitor::MY_NEW_MONITOR>{
                address.value(), value, tid, pc, time});
        }
    }
}
// or, again, if you have read and write called at separate places:
void HMVThreadMonitor::my_new_monitor_read(const o3::DynInstPtr& inst) {
    if (!started) {
        return;
    }
    auto time = controller.current_time();
    if (auto address = controller.match_address(inst->physEffAddr)) {
        auto pc = normalize_pc(inst);
        auto value = get_value(inst);
        controller.push_event(ReadAt<Monitor::MY_NEW_MONITOR>{
            address.value(), value, tid, pc, time});
    }
}

void HMVThreadMonitor::my_new_monitor_write(const o3::DynInstPtr& inst) {
    if (!started) {
        return;
    }
    auto time = controller.current_time();
    if (auto address = controller.match_address(inst->physEffAddr)) {
        auto pc = normalize_pc(inst);
        auto value = get_value(inst);
        controller.push_event(WriteAt<Monitor::MY_NEW_MONITOR>{
            address.value(), value, tid, pc, time});
    }
}
```

Depending in which part of the execution flow your monitor is placed, the value of a write may not be available using `get_value(inst)`, in which case you probably want to use `inst->hmvAvailabilityData`.
See `HMVThreadMonitor::local_commit` for an example of this happening.

Finally, all you need to do is call your new monitor in the adequate places inside gem5. Something like this:
```cpp
    if (cpu->hmvThreadMonitor && inst->isLoad()) {
        cpu->hmvThreadMonitor->my_new_monitor_read(inst);
    }
```

Make sure to update [`src/hmv/integration/gem5/base.patch`](../integration/gem5/base.patch) with all code changes done inside gem5.
