# HOME-V

HOME-V is an open-source framework for multicore design verification which focuses on the validation of the interaction between cores through coherent shared memory. Not only it can accommodate existent approaches but can also support the development of novel
techniques. To broaden potential usage, HOME-V was combined with gem5, an open-source simulation environment used worldwide by academic researchers and major technology companies. The framework is made available with a collection of integrated tools (generators of concurrent test programs and memory model checkers).
HOME-v provides **H**igh **O**bservability of **M**emory **E**vents for **V**erification by means of a  general observability template for memory events. Such template can be tailored to the needs of distinct verification approaches reported in the literature. Besides, it provides an architecture-independent test format for describing abstract tests and an instruction-set architecture encoder that converts the abstracts tests into binaries without the need for compilation. Currently, ARMV8 and RISC-V targets are supported.

The following resources are available:
* Checkers:
    * [`LCHECK`](src/hmv/checker/lcheck.hpp) - a rebuild of an inference checker ([reference paper](https://doi.org/10.1109/HPCA.2009.4798276)) without the backtracking algorithm.
    * [`SRSB`](src/hmv/checker/srsb.hpp) - a rebuild of a value checker called Relaxed Scoreboard ([reference paper](https://doi.org/10.1109/MICRO.2008.4771799)).
    * [`MRSB`](src/hmv/checker/mrsb.hpp) - a rebuild of a order checker called Multiple Relaxed Scoreboards ([reference paper](https://doi.org/10.7873/DATE.2013.138)).
    * [`EVCH`](src/hmv/checker/evch.hpp) - a version with minor changes of the order checker EveCheck ([reference article](https://doi.org/10.1109/TCAD.2022.3178051)), targeting designs relying on protocols imposing strict store visibility, such as gem5's `MOESI_CMP_directory`.
    * [`HOCH`](src/hmv/checker/hoch.hpp) - an order checker similar to EveCheck, targeting designs relying on protocols allowing relaxed store visibility, such as gem5's `MESI_Three_Level`.
* Test generators:
    * [`Random`](src/hmv/generator/random.py) - there are four combinations of random generators:
        * `RnR` - Full Random test generator (Random threads + Random addresses).
        * `RnB` - Random Biasing test generator (Random threads + Biasing addresses, see `CnB` for Biasing reference).
        * `CnR` - Random Chaining test generator (Chaining threads + Random addresses, see `CnB` for Chaining reference).
        * `CnB` - Random Chaining and Biasing test generator (Chaining Threads + Biasing addresses, [reference article](https://doi.org/10.1109/TCAD.2019.2894376)).
    * [`SCA`](src/hmv/generator/sca.py) - Steep Coverage-Ascent ([reference paper](http://doi.org/10.1145/3240765.3240852)).
    * [`HTG`](src/hmv/generator/htg.py) - Hybrid Test Generator ([reference article](http://doi.org/10.1109/TCAD.2020.2974343)).
    * [`McVerSi`](src/hmv/generator/mcversi.py) - genetic programming generator ([reference paper](https://doi.org/10.1109/HPCA.2016.7446099)).
* Coverage metrics:
    * [`FSM_STRUCTURAL`](src/hmv/coverage/fsm_structural.hpp): structural coverage of FSM transitions (specifically FSMs composing the cache protocol).
    * [`FSM_PER_INSTANCE`](src/hmv/coverage/fsm_per_instance.hpp): extended structural coverage of FSM transitions, differentiating each instance of a FSM.
* Platforms:
    * [`gem5`](src/hmv/platform/gem5.py): [gem5 simulator](https://github.com/gem5/gem5), patched to execute HOME-V's checkers and coverage metrics.
* Artificial error injection:
    * Some artificial errors are provided for gem5's `MESI_Two_Level`, `MESI_Three_Level`, and `MOESI_CMP_directory` cache protocols.

**DISCLAIMERS**:
* The implementations provided by HOME-V **should NOT** be interpreted as an exact match of the internal implementations used in the production of their respective papers/articles (unless explicitly stated).
* If you use HOME-V in your research, make sure to cite all papers/articles of the underlying techniques you're using and specify this github repository as the source for the implementations.
* `McVerSi` implementation is based on code extracted from the author's open source [repository](https://github.com/melver/mc2lib) and a [patch](https://www.marcoelver.com/files/mcversi_gem5.patch) file provided in his [website](https://www.marcoelver.com).
* `McVerSi` has an error in the tournament algorithm (during the evaluation of each pair of mates), this error is from the original source code (check the [source file](src/hmv/generator/component/mcversi/test_pool.cpp) for more details).
* `McVerSi` does not provide an implementation for its "adaptive coverage" nor a full explanation of how it is supposed to work, so HOME-V does not attempt to recreate it (a flat coverage metric is used instead).

## Dependencies

A [Dockerfile](Dockerfile) based on Ubuntu 24.04 is provided to satisfy all system dependencies.
Some python dependencies are expected to be satisfied by a virtual environment.
If you want to run HOME-V directly in your host system, make sure to install all packages that are listed inside the Dockerfile.

## Quickstart

### Setup

Clone the repository to start the set up:
```bash
git clone https://github.com/homevorg/homev.git
```
It is recommended to use a container created with the provided Dockerfile to handle all dependencies.
Get inside the repository and build the docker image:
```bash
cd homev
docker build . -t ubuntu-home-v
```
Now run a container using the newly created image, mounting the repository's path to be accessible inside it:
```bash
docker run -u $UID:$GID --volume $(pwd):/homev --rm -it ubuntu-home-v
cd /homev
```
Next, create a virtual environment and activate it to manage all python dependencies:
```bash
python3 -m venv .venv
source .venv/bin/activate
```
Now you can install HOME-V. If you just want to use it, install it as follows:
```bash
pip install .
```
Alternatively, if you're planning to modify/extend HOME-V:
```bash
pip install nanobind scikit-build-core[pyproject]
pip install --no-build-isolation -ve . -Ceditable.rebuild=true
```
Installing it this way enables automatic recompilation if you run HOME-V after any changes.
However, changes that affect checkers or other parts of HOME-V that are used inside the platform still require you to manually recompile it using `hmv-build`.
The next step is to compile the platform (gem5 is currently the only option):
```bash
hmv-build -j12 gem5 release
```
Replace `-j12` with whichever amount of jobs you wish to use to compile gem5. If you need debug capabilities, compile gem5 in debug mode:
```bash
hmv-build -j12 gem5 debug
```

### Run

The command to run tests is `hmv-run`. It has several flags with default values, check `--help/-h` for more detailed information.
The overall format is `hmv-run <general options> GENERATOR <generator options> PLATFORM <platform options>`.
For instance, if you want to run a test suite using the `HOCH` checker, `FSM_STRUCTURAL` coverage metric, CPU with 8 cores, `RISCV` architecture, `MESI3` preset, `SCA` generator with `N = [512, 2048]`, `S = [16, 128]`, `COVERAGE` variant and 5 execution instances:
```bash
hmv-run -c HOCH -m FSM_STRUCTURAL -p 8 -A RISCV -P MESI3 SCA -n 512 -N 2048 -s 16 -S 128 -v COVERAGE -i 5 gem5
```

You can run multiple random seeds consecutively (`-r/--run-count`), choose from which seed in the sequence to start from (`-s/--start-from`), and/or specify the main seed (`-S/--master-seed`) from which everything else is derived.

### Output

All results are saved inside the results directory (named `results` by default), using checker, generator, and platform parameters to compose an unique directory name.
Inside each unique directory, one directory for each seed is created.
If the seed directory already exists, HOME-V assumes the results for the seed are ready and skips the execution of it (use `-o/--overwrite` to prevent the skip).
Considering the example above, you would see the following:
```bash
results
└── HOCH.FSM_STRUCTURAL.RISCV.MESI3.p8.w1.SCA.COVERAGE.n512_2048.s16_128.a6.i5.gem5.NONE
    └── 13271066930465087934
        ├── FSM_STRUCTURAL.csv
        ├── FSM_STRUCTURAL.dat
        ├── parameters.toml
        ├── platform
        │   ├── input
        │   │   ├── parameters.toml
        │   │   ├── test.dat
        │   │   └── test_workload
        │   └── output
        │       └── output.dat
        └── tests.csv
```
All result directories follow the same overall structure. Inside a seed directory you will have:
* `<COVERAGE METRIC>.csv`: headless CSV file containing the final aggregated coverage data.
* `<COVERAGE METRIC>.dat`: binary file containing the final aggregated coverage data in serialized format.
* `parameters.toml` TOML file containing all parameters passed to `hmv-run` to execute the run.
* `platform`: directory containing all input/output from/to the platform (constantly updated during a run).
    * `input`: directory containing the HOME-V input provided to the platform.
        * `parameters.toml`: TOML file containing the platform's parameters (invariant between tests).
        * `test.dat`: binary file containing the serialized test object of the **last** test executed.
        * `test_workload`: generic binary responsible for loading and executing each test provided by the generator.
    * `output`: directory containing the HOME-V output provided by the platform for the **last** executed test.
        * `output.dat`: binary file containing the results, such as coverage data, timings, and checker's error code.
* `tests.csv`: CSV file containing the results for each test in the run.

### Configuration file

There are parameters that would be too cumbersome to define with command line options.
To address this issue, `hmv-run` supports the concept of presets (`--preset/-P`) and a custom configuration file (`--config-file/-C`).
Please refer to the default [configuration file](src/hmv/resources/configuration.toml) for a reference of all available parameters.

Presets allow you to overwrite any parameters inside the `[runner]` section, but also contain parameters that would otherwise be undefined.
The following default presets are provided:
* `MESI2`: `MESI_Two_Level` protocol, 64-byte cache blocks, with:
    * L1 - 64 KiB, 4-way private cache.
    * L2 - 4 MiB, 16-way shared cache.
* `MESI3`: `MESI_Three_Level` protocol, 64-byte cache blocks, with:
    * L1 - 4 KiB, 1-way private cache.
    * L2 - 64 KiB, 4-way private cache.
    * L3 - 4 MiB, 16-way shared cache.
* `MOESI`: `MOESI_CMP_directory` protocol, 64-byte cache blocks, with:
    * L1 - 64 KiB, 4-way private cache.
    * L2 - 4 MiB, 16-way shared cache.

Note that preset is the only parameter from the configuration file that is reflected in the unique directory name to store the results.
If you want to use a custom configuration file, it is a good practice to only define new presets, this way you can still differentiate the results of distinct configurations.

## Contributing

Everyone is welcomed to contribute. Just make sure to follow HOME-V's formatting rules: use [`clang-format`](https://clang.llvm.org/docs/ClangFormat.html) for C++ and [`Black`](https://github.com/psf/black) for python.
There are several `README.md` files throughout the source tree with tutorials on how to add new components, you should start from [here](src/hmv/README.md).
