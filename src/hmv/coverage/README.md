# HOME-V coverage metrics

## How to add a new coverage metric

All coverage metrics are expected to work with FSM transitions to gather coverage data.
If you want to create a coverage metric based on a different type of information, it will require extensive changes that are not covered in this document.

Each coverage metric has to provide a single top level entity to interface with.
Your new metric should have a single header/source pair inside `src/hmv/coverage` containing the top level class.
If you want to have more header/source files, you can create a `src/hmv/coverage/component/MY_METRIC/` subdirectory to store them.
If you're defining reusable components shared between multiple coverage metrics, you may put them directly inside `src/hmv/coverage/component`.
All coverage metric related code and classes should be declared inside the `hmv::coverage` namespace.

### Required includes

Two includes from the standard library are required:
```cpp
#include <fstream> // used by serialize/deserialize methods
#include <string> // used by push method
```

And one include from HOME-V:
```cpp
#include "hmv/core/types.hpp" // for hmv::Address definition used by push method
```

### Mandatory methods

There are eight mandatory methods that you need to implement inside the top level class of your new metric.

#### Constructor

The only requirement constructor-wise is to be default constructable. Either make sure your class has an implicit default constructor or declare one explicitly:
```cpp
namespace hmv::coverage {
// ...
    class MyNewMetric {
    // ...
     public:
        MyNewMetric() = default;
    // ...
    };
// ...
} // namespace hmv::coverage
```

#### `push`

The main method, called every time a transition is triggered.
It receives three strings with the name of the FSM, the state, and the event.
It also receives the address related to the transition (the cache line address) and an unsigned indicating in which instance of the FSM the transition was triggered.
Full signature:
```cpp
void MyNewMetric::push(const std::string& fsm,
                       const std::string& state,
                       const std::string& event,
                       Address address,
                       unsigned instance);
```

#### `affects_value`

This method receives another metric object and should return true if either `merge` or `remove` would affect the `value` returned.
Full signature:
```cpp
bool MyNewMetric::affects_value(const MyNewMetric& other) const;
```

#### `merge`

This method receives another metric to merge with.
Full signature:
```cpp
void MyNewMetric::merge(const MyNewMetric& other);
```

#### `remove`

This method receives another metric to remove from the stored values.
It must operate as the reverse of `merge`.
Full signature:
```cpp
void MyNewMetric::remove(const MyNewMetric& other);
```

#### `reset`

This method resets all coverage data collected.
Full signature:
```cpp
void MyNewMetric::reset();
```

#### `value`

This method returns the current coverage value.
Full signature:
```cpp
size_t MyNewMetric::value() const;
```

#### `serialize`

This method is responsible for serializing all the internal data of your metric to a binary file.
Make use of the `serialize` functions provided by [`hmv/utility/filesystem.hpp`](../utility/filesystem.hpp) to simplify your code.
Full signature:
```cpp
void MyNewMetric::serialize(std::ofstream& file) const;
```

#### `deserialize` (static method)

This method is responsible for deserializing all the internal data of your metric from a binary file to an object.
Make use of the `deserialize` functions provided by [`hmv/utility/filesystem.hpp`](../utility/filesystem.hpp) to simplify your code.
Full signature:
```cpp
MyNewMetric MyNewMetric::deserialize(std::ifstream& file);
```

### Platform-independent changes

Add your new metric to the `hmv::coverage::Type` enumeration in [`src/hmv/coverage/metric.hpp`](metric.hpp):
```cpp
    enum class Type : uint8_t {
        FSM_STRUCTURAL = 0,
        FSM_PER_INSTANCE = 1,
        // ...
        MY_NEW_METRIC = 3,
    };
```

And add a case statement to instantiate your new metric in the two constructors of `coverage::Metric` in [`src/hmv/coverage/metric.cpp`](metric.cpp):
```cpp
Metric::Metric(Type type, size_t max) : tag{type}, max_value{max} {
    switch (tag) {
    case Type::FSM_STRUCTURAL:
        model = std::make_unique<Model<FSMStructural>>(FSMStructural{});
        break;
    // ...
    case Type::MY_NEW_METRIC:
        model = std::make_unique<Model<MyNewMetric>>(MyNewMetric{});
        break;
    }
    assert(model);
}

Metric::Metric(Type type, size_t max, std::ifstream& file)
    : tag{type}, max_value{max} {
    switch (tag) {
    case Type::FSM_STRUCTURAL:
        model = std::make_unique<Model<FSMStructural>>(file);
        break;
    // ...
    case Type::MY_NEW_METRIC:
        model = std::make_unique<Model<MyNewMetric>>(file);
        break;
    }
    assert(model);
}
```

You also need to add some boilerplate virtual functions (`affects_value`, `merge`, and `remove`) in [`src/hmv/coverage/metric.hpp`](metric.hpp)::
```cpp
    class Metric {
    // ...
        class Concept {
        // ...
            virtual bool affects_value(const MyNewMetric& other) const = 0;
            // ...
            virtual void merge(MyNewMetric& other) const = 0;
            // ...
            virtual void remove(MyNewMetric& other) const = 0;
        // ...
        };

        template <typename M>
        class Model final : public Concept {
        // ...
            bool affects_value(const MyNewMetric& other) const override;
            // ...
            void merge(MyNewMetric& other) const override;
            // ...
            void remove(MyNewMetric& other) const override;
        // ...
        };
    };
    // ...
    template <typename M>
    inline bool Metric::Model<M>::affects_value(const MyNewMetric& other) const {
        if constexpr (std::is_same<M, MyNewMetric>::value) {
            return other.affects_value(metric);
        }
        assert(false);
        return false;
    }
    // ...
    template <typename M>
    void Metric::Model<M>::merge(MyNewMetric& other) const {
        if constexpr (std::is_same<M, MyNewMetric>::value) {
            other.merge(metric);
        } else {
            assert(false);
        }
    }
    // ...
    template <typename M>
    void Metric::Model<M>::remove(MyNewMetric& other) const {
        if constexpr (std::is_same<M, MyNewMetric>::value) {
            other.remove(metric);
        } else {
            assert(false);
        }
    }
    // ...
```

Finally, update the python binding in [`src/hmv/bindings/nanobind.cpp`](../bindings/nanobind.cpp):
```cpp
    nb::enum_<coverage::Type>(coverage, "Type")
        .value("FSM_STRUCTURAL", coverage::Type::FSM_STRUCTURAL)
        // ...
        .value("MY_NEW_COVERAGE", coverage::Type::MY_NEW_COVERAGE)
        .export_values();
```

### Platform-specific changes

Some platform-specific changes are required to make your new coverage metric available in them.

#### gem5 integration

Add your new metric to the `HMVCoverageMetric` enumeration in [`src/hmv/integration/gem5/HMVController.py`](../integration/gem5/HMVController.py):
```python
class HMVCoverageMetric(ScopedEnum):
    # Keep this list in sync with hmv::coverage::Type
    vals = ["FSM_STRUCTURAL", "FSM_PER_INSTANCE", ..., "MY_NEW_METRIC"]
```

And add an if statement to calculate the maximum value of your metric inside `_coverage_max` function in [`src/hmv/platform/gem5.py`](../platform/gem5.py):
```python
    def _coverage_max(self, metric):
        if metric == coverage.Type.FSM_STRUCTURAL:
            return sum([cache["transition_count"] for cache in self.caches])
        elif metric == coverage.Type.FSM_PER_INSTANCE:
        # ...
        elif metric == coverage.Type.MY_NEW_METRIC:
            # calculate and return the maximum value of your metric here
        return 1
```
