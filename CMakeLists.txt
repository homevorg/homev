cmake_minimum_required(VERSION 3.28)

project(hmv LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT SKBUILD)
  message(WARNING "\
  This CMake file is meant to be executed using 'scikit-build'. Running
  it directly will almost certainly not produce the desired result. If
  you are a user trying to install this package, please use the command
  below, which will install all necessary build dependencies, compile
  the package in an isolated environment, and then install it.
  =====================================================================
   $ pip install .
  =====================================================================
  If you are a software developer, and this is your own package, then
  it is usually much more efficient to install the build dependencies
  in your environment once and use the following command that avoids
  a costly creation of a new virtual environment at every compilation:
  =====================================================================
   $ pip install nanobind scikit-build-core[pyproject]
   $ pip install --no-build-isolation -ve .
  =====================================================================
  You may optionally add -Ceditable.rebuild=true to auto-rebuild when
  the package is imported. Otherwise, you need to re-run the above
  after editing C++ files.")
endif()

# Try to import all Python components potentially needed by nanobind
find_package(Python 3.12
  REQUIRED COMPONENTS Interpreter Development.Module
  OPTIONAL_COMPONENTS Development.SABIModule)
# Import nanobind through CMake's find_package mechanism
find_package(nanobind CONFIG REQUIRED)
# Import xxhash through pkgconfig
# (ubuntu doesn't install find_package for it)
find_package(PkgConfig REQUIRED)
pkg_check_modules(xxHash REQUIRED IMPORTED_TARGET libxxhash)

# We are now ready to compile the actual extension module
nanobind_add_module(
  # Name of the extension
  _hmv_internal
  STABLE_ABI
  NB_STATIC
  # Source code goes here
  src/hmv/bindings/nanobind.cpp
  src/hmv/core/event.cpp
  src/hmv/core/operation.cpp
  src/hmv/core/register_allocator.cpp
  src/hmv/core/test.cpp
  src/hmv/core/trace.cpp
  src/hmv/coverage/fsm_per_instance.cpp
  src/hmv/coverage/fsm_structural.cpp
  src/hmv/coverage/metric.cpp
  src/hmv/generator/htg.cpp
  src/hmv/generator/mcversi.cpp
  src/hmv/generator/sca.cpp
  src/hmv/generator/component/biasing.cpp
  src/hmv/generator/component/chaining.cpp
  src/hmv/generator/component/random_address.cpp
  src/hmv/generator/component/random_thread.cpp
  src/hmv/generator/component/mcversi/crossover.cpp
  src/hmv/generator/component/mcversi/fitaddrs_collector.cpp
  src/hmv/generator/component/mcversi/test.cpp
  src/hmv/generator/component/mcversi/test_pool.cpp
  src/hmv/platform/output.cpp
  src/hmv/platform/traits.cpp
  src/hmv/platform/component/signals.cpp
  src/hmv/utility/filesystem.cpp
  )
target_include_directories(_hmv_internal PRIVATE src/)
target_compile_options(_hmv_internal PRIVATE "-std=c++23")
target_link_libraries(_hmv_internal PRIVATE PkgConfig::xxHash)

# Install directive for scikit-build-core
install(TARGETS _hmv_internal LIBRARY DESTINATION hmv)
