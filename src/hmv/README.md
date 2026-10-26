# HOME-V's Developer Guide

This guide will help you implement the most common components inside HOME-V, such as checkers, generators, and coverage metrics.
Before you dive into gritty details about how to implement them, first take a look at the directory hierarchy of the source code:
* `application`: contains python classes and entry points of all HOME-V applications (`hmv-build`, `hmv-run`).
* `bindings`: contains bindings to different languages (so far only python is supported, see [`nanobind.cpp`](bindings/nanobind.cpp)).
* `checker`: contains all implementation related to checkers.
* `core`: contains core types used throughout the project.
* `coverage`: contains all implementation of coverage metrics.
* `encoder`: contains all implementation of ISA encoders (responsible for converting a high-level `Operation` to machine code).
* `generator`: contains all implementation of generators.
* `integration`: contains external code (i.e. not compiled as part of HOME-V) required to integrate platforms and run tests.
* `platform`: contains internal code required to integrate and deploy platforms.
* `resources`: contains resources such as default configuration files and examples.
* `utility`: contains generic utility code that has no dependency with other parts of HOME-V.

## Component-specific guides

* [Overview of core types](core/README.md#overview-of-core-types)
* [How to add a new checker](checker/README.md#how-to-add-a-new-checker)
* [How to add a new coverage metric](coverage/README.md#how-to-add-a-new-coverage-metric)
* [How to add a new generator](generator/README.md#how-to-add-a-new-generator)
* [How to add a new monitor](checker/README.md#how-to-add-a-new-monitor)
