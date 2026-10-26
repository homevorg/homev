# HOME-V generators

All generators supported by HOME-V are implemented inside this directory.
The following section will guide you on how to add a new generator.

## How to add a new generator

While most components of HOME-V require a C++ implementation, generators require a python implementation instead.
You're also free to implement part (or most) of your generator in C++, bind it to python, and implement a wrapper in python to fulfill the interface requirements.

Each generator need to have a python module in `src/hmv/generator` containing a top level class inheriting the python class `Generator` defined in [`common.py`](common.py).
If you want to have more modules or header/source files, you can create a `src/hmv/generator/component/mynewgenerator/` subdirectory to store them.
If you're defining reusable components shared between multiple generators, you may put them directly inside `src/hmv/generator/component`.
All generator related C++ code and classes should be declared inside the `hmv::generator` namespace.

## Required class fields

There are three class fields that your generator must have:
* `name`: string with the short name of your generator, usually an abbreviation (don't use spaces or special characters). It is used as the name to select your generator in `hmv-run`.
* `description`: string with the description of your generator, usually the unabbreviated name (spaces and special characters are allowed). It is used as the description of your generator in `hmv-run`.
* `configuration_key`: string with the configuration key of your generator, usually the same as the name. It defines the section for your generator in the configuration file.

For instance, you may have something like this:
```python
from .common import Generator as BaseGenerator

class Generator(BaseGenerator):
    name = "MNG"
    description = "My New Generator"
    configuration_key = "MNG"
    # ...
```

And add a new section in the [default configuration file](../resources/configuration.toml):
```toml
# ...
[runner.MNG]
# add configuration keys for your generator here
```

## Required methods

There are twelve methods that your generator must override when inhering from the `common.Generator` class.

### `__init__`

The constructor always receives the same five parameters:
* `memory_traits`: a [`MemoryTraits`](../core/README.md#memorytraits) object with the memory traits to be used.
* `cache_traits`: a [`CacheTraits`](../core/README.md#cachetraits) object with the cache traits of the last level cache.
* `thread_count`: number of threads to generate per test.
* `metrics`: metrics that will be provided in the `feedback`.
* `configs`: table with all configurations defined for your generator.
* `options`: keyword arguments (kwargs) defined by the `define_options` class method.

Make sure to call the super constructor as follows:
```python
    def __init__(self, memory_traits, cache_traits, thread_count, metrics, configs, **options):
        super().__init__(memory_traits, cache_traits, thread_count, metrics)
        # initialize your object fields here
        # ...
```

### `__call__`

This method generates and returns a new [`Test`](../core/README.md#test) to run.
If your generator can't generate a new test, it should return `None` instead.
If you implemented your generator in C++, you can use `std::optional<hmv::Test>` as the return value to allow a compatible semantic.
Full signature:
```python
    def __call__(self):
```

You probably want to save here the relevant values of the generated test to later return in [`field_values`](#field_values) method.


If you're implementing the generator purely in python, you can use native python lists to create the list of addresses, the list of operations for each thread, and the list of threads.
In the end, all you need to do is feed these lists to the `Test` constructor:
```python
    def __call__(self):
        addresses = []
        threads = []
        # populate the addresses and threads
        # for each thread, create a python list of operations
        # (use Load, Store, Fence, Nop to create operations)
        # ...
        # and finally return a test created with the populated python lists
        return Test(addresses, threads, number_of_instances)
```

### `feedback`

This method processes the feedback of the execution of the last test returned by [`__call__`](#__call__).
Return `True` if the generator achieved its goals (e.g. 100% aggregate coverage), otherwise return `False`.
Full signature:
```python
    def feedback(self, output):
```

The parameter `output` is an object of the type `platform.Output` (or `hmv::platform::Output` in C++).
It is a struct with the following fields:
* `test_time`: time (in nanoseconds) that took to run the test.
* `warmup_time`: time (in nanoseconds) that took to run the warmup execution instances.
* `checkers`: vector of `platform.Output.Checker` (or `hmv::platform::Output::Checker` in C++) objects, with the results of each checker enabled.
* `metrics`: vector of `coverage.Metric` (or `hmv::coverage::Metric` in C++) objects with the coverage collected from the current test.
* `aggregated_metrics`: vector of `coverage.Metric` (or `hmv::coverage::Metric` in C++) objects with the coverage aggregated from all tests so far.
* `trace`: `Trace` object with the events requested by the optional [`trace_selection`](#trace_selection) method.

The `platform.Output.Checker` is a struct with the following fields:
* `type`: the checker that generated this output (a value from `hmv::checker::Type` enumeration).
* `code`: the error code of the checker (0 = no error).
* `time`: the time (in nanoseconds) that took to execute the checker (this time is included in the `test_time`).

### `fields`

This method returns a list with the names of the fields that are associated with a test (e.g. test size, number of addresses, etc).
The order of the fields in this list will be the order in the resulting CSV file.
Full signature:
```python
    def fields(self):
```

### `field_values`

This method returns a dict with the fields and values for the last test generated by [`__call__`](#__call__).
Full signature:
```python
    def field_values(self):
```

### `max_address_count`

This method returns the maximum amount of addresses a single test may use.
The platform requires this information to allocate a vector of addresses sufficiently large to fit all of them.
Full signature:
```python
    def max_address_count(self):
```

### `max_thread_size`

This method returns the maximum amount of operations a thread may have.
Similar to [`max_address_count`](#max_address_count), the platform requires this information to allocate sufficient space to store all operations of a thread.
Full signature:
```python
    def max_thread_size(self):
```

If you're using `Nop` operations with a non-default amount of nops, make sure to account for it in the maximum amount of operations.
Other than that, you don't need to worry about how many machine code operations a single `Operation` will become.

### `parameters`

This method returns a dict with all parameters of your generator. It should reflect all `options` received in the constructor.
Full signature:
```python
    def parameters(self):
```

### `set_seed`

This method prepares the generator to start generating tests with the specified `seed`.
You may need to (re)initialize the internal state of your generator here.
Full signature:
```python
    def set_seed(self, seed):
```

One common practice is to have the actual generator in a separate class, and only instantiate it in this method.
For instance, this is how the `SCA` generator is implemented:
```python
    def set_seed(self, seed):
        self.sca = SCA(
            self.variant,
            self.memory_traits,
            self.cache_traits,
            self.thread_count,
            self.n_range.to_exponent_range(),
            self.s_range.to_exponent_range(),
            self.abc,
            self.sbc,
            self.instances,
            Chaining.Mix(self.mix),
            seed,
        )
```

The `SCA` class is the actual generator, while the `Generator` class acts as a wrapper to manage the options, configuration, and instantiation of `SCA`.

### `unique_id`

This method returns a string to be used as the unique directory for the current run.
Full signature:
```python
    def unique_id(self):
```

It is usually composed by the [`name`](#required-class-fields) field concatenated with all [`parameters`](#parameters) of the generator.
For instance, this is how `McVerSi` implements it:
```python
    def unique_id(self):
        return "{}.n{}.a{}.i{}".format(
            self.name,
            self.test_size,
            self.stride,
            self.instances,
        )
```

### `test_display`

This method returns a string to be displayed in the terminal with information of the current test.
It usually displays the same data that is returned by [`field_values`](#field_values).
Full signature:
```python
    def test_display(self):
```

### `define_options`

This method defines all command line options required by the generator.
It receives a `parser` object from the `argparse` library.
Full signature:
```python
    @classmethod
    def define_options(cls, parser):
```

## Optional methods

In addition to the required methods, you can also override the following method.

### `trace_selection`

This method returns a [`TraceSelection`](../core/README.md#trace) required by the generator.
This trace selection will be used to populate the `trace` field in the `platform.Output` object provided as argument in the [`feedback`](#feedback) method.
Full signature:
```python
    def trace_selection(self):
```

## Platform-independent changes

First, import the module of your new generator in [`src/hmv/application/run.py`](../application/run.py):
```python
from hmv.generator import htg, mcversi, random, sca, udt, mynewgenerator
```

And then add your generator to the list of generators:
```python
    _generators = [
        htg.Generator,
        mcversi.Generator,
        random.RnB,
        random.RnR,
        random.CnB,
        random.CnR,
        sca.Generator,
        udt.Generator,
        mynewgenerator.Generator,
    ]
```
