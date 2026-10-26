# Copyright (C) 2025-2026 Marleson Graf

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

from contextlib import chdir
from importlib import resources
import os
import shlex
import shutil
import signal
import subprocess
import tomlkit

from .common import BaseBuilder, BaseDispatcher
from hmv import coverage, CacheTraits, ISA
from hmv.platform import signal_continue, Traits

class Dispatcher(BaseDispatcher):
    name = "gem5"
    description = "gem5 simulator"
    configuration_key = "gem5"
    _m5out_dir = os.path.join(Traits.platform_dir, "m5out")
    _main_file = "gem5/testsuite/main.cpp"
    _log_file = os.path.join(Traits.output_dir, "log.txt")
    _params_file = os.path.join(Traits.input_dir, "parameters.toml")
    _script_file = "gem5/configs/se.py"
    _workload_file = os.path.join(Traits.input_dir, "test_workload")

    def __init__(
        self,
        install_dir,
        memory_traits,
        architecture,
        core_count,
        warmup_instances,
        checkers,
        metrics,
        details,
        configs,
        **options,
    ):
        super().__init__(
            install_dir,
            memory_traits,
            architecture,
            core_count,
            warmup_instances,
            checkers,
            metrics,
            details,
        )
        assert (
            self.memory_traits.address_size <= 33
        ), "maximum supported address size is 33 bits"
        self.params = configs["parameters"]
        self.params["CPU"]["architecture"] = self.architecture.name
        self.params["CPU"]["core_count"] = self.core_count
        self.params["checkers"] = [c.name for c in self.checkers]
        self.params["metrics"] = [m.name for m in self.metrics]
        self.params["address_size"] = self.memory_traits.address_size
        self.params["word_size"] = self.memory_traits.word_size.name
        self.params["fault"] = options["fault"]
        self.params["warmup_instances"] = self.warmup_instances
        self.standalone = options["standalone"]
        self.debug = options["debug"]
        self.test_compiler = configs[self.architecture.name]["test_compiler"]
        self.test_sources = configs[self.architecture.name]["test_sources"]
        self.test_flags += configs[self.architecture.name]["test_flags"]
        self.gem5, has_debug = Builder.get_binary(self.install_dir, self.debug)
        self.script_flags = ["-p", self._params_file, "-w", self._workload_file]
        self.flags = [
            "--outdir",
            self._m5out_dir,
            "--listener-mode",
            "off",
        ] + shlex.split(options["gem5_flags"])
        if self.debug and has_debug:
            self.flags.append("--debug-flags=HMV")
        if self.details:
            self.script_flags.append("-d")
        self.outfile = subprocess.DEVNULL

    def command(self, directory):
        return (
            [self.gem5]
            + self.flags
            + [os.path.join(directory, self._script_file)]
            + self.script_flags
        )

    def deploy(self, seed):
        Traits.makedirs()
        with open(self._params_file, "w") as file:
            tomlkit.dump(self.params, file)
        self.test_loader = self.spawn_test_loader()
        self.wait_continue()

    def execute(self, test):
        if not self.standalone:
            signal_continue(self.test_loader.pid, test.instances)
            self.wait_continue()
            return self.test_loader.returncode or 0
        return self.execute_standalone(test)

    def execute_standalone(self, test):
        self.outfile = self.open_output_file()
        with resources.as_file(resources.files("hmv.integration")) as integration_dir:
            process = subprocess.run(
                self.command(integration_dir),
                stdout=self.outfile,
                stderr=self.outfile,
            )
        return process.returncode

    def destroy(self):
        self.destroy_test_loader()
        if not self.debug:
            shutil.rmtree(self._m5out_dir, ignore_errors=True)

    def get_last_level_cache_traits(self):
        units = {"B": 1, "KiB": 1024, "MiB": 1024 * 1024, "GiB": 1024 * 1024 * 1024}
        llc = "L3" if "L3" in self.params["Ruby"] else "L2"
        value = int("".join(filter(str.isdigit, self.params["Ruby"][llc]["size"])))
        unit = "".join(filter(str.isalpha, self.params["Ruby"][llc]["size"]))
        assert unit in units, "Invalid unit detected in {} cache size".format(llc)
        return CacheTraits(
            value * units[unit],
            self.params["Ruby"]["block_size"],
            self.params["Ruby"][llc]["associativity"],
        )

    def set_trace_selection(self, trace_selection):
        super().set_trace_selection(trace_selection)
        if trace_selection:
            self.params["trace_selection"] = dict()
            for op, monitors in self.trace_selection.items():
                self.params["trace_selection"][op.name] = [m.name for m in monitors]

    def set_max_thread_size(self, size):
        self.params["max_thread_size"] = size

    def parameters(self):
        return {"fault": self.params["fault"]}

    def unique_id(self):
        return "{}{}.{}".format(
            self.name, "-sa" if self.standalone else "", self.params["fault"]
        )

    def spawn_test_loader(self):
        self.outfile = self.open_output_file()
        with resources.as_file(resources.files("hmv.integration")) as integration_dir:
            self.compile(
                self._sources(integration_dir), self._workload_file, integration_dir
            )
            if not self.standalone:
                self.script_flags += ["-g", str(os.getpid())]
                return subprocess.Popen(
                    self.command(integration_dir),
                    stdout=self.outfile,
                    stderr=self.outfile,
                )
        return None

    def destroy_test_loader(self):
        if self.test_loader and not self.test_loader.returncode:
            signal_continue(self.test_loader.pid, 0)
            self.test_loader.wait()

    def wait_continue(self):
        if not self.test_loader:
            return
        signal.pthread_sigmask(signal.SIG_BLOCK, {signal.SIGUSR1})
        while not signal.sigtimedwait({signal.SIGUSR1}, 1.0):
            if returncode := self.test_loader.poll():
                break
        signal.pthread_sigmask(signal.SIG_UNBLOCK, {signal.SIGUSR1})

    def open_output_file(self):
        if self.debug:
            return open(self._log_file, "w")
        return subprocess.DEVNULL

    @classmethod
    def define_options(cls, parser):
        parser.add_argument(
            "-F",
            "--fault",
            default="NONE",
            help="fault to inject",
        )
        parser.add_argument(
            "-G",
            "--gem5-flags",
            default="",
            help="flags to forward to gem5",
        )
        parser.add_argument(
            "-s",
            "--standalone",
            action="store_true",
            help="use standalone test execution",
        )
        parser.add_argument(
            "-d", "--debug", action="store_true", help="enable debug output"
        )

    def _sources(self, directory):
        return [os.path.join(directory, file) for file in self.test_sources] + [
            os.path.join(directory, self._main_file)
        ]


class Builder(BaseBuilder):
    name = Dispatcher.name
    description = Dispatcher.description
    _build_dir = "build/ALL"
    _targets = {"release": "gem5.fast", "debug": "gem5.opt"}

    def __init__(self, configs, install_dir, target):
        super().__init__(configs, install_dir)
        assert target in self._targets.keys(), "unsupported gem5 target"
        self.version = self.configs["gem5"]["version"]
        self.git_url = self.configs["gem5"]["git_url"]
        self.target_bin = self._targets[target]
        self.patches = self.configs["gem5"]["patches"]

    def __call__(self, job_count):
        project_dir = os.path.join(self.install_dir, self.name)
        with resources.as_file(resources.files("hmv")) as hmv_dir:
            if not os.path.exists(project_dir):
                self._clone_and_patch(hmv_dir)
            if not os.path.exists(os.path.join(project_dir, self._build_dir)):
                self._config_build(project_dir)
            self._build(project_dir, hmv_dir, job_count)

    @classmethod
    def get_binary(cls, install_dir, debug):
        debug_bin = os.path.join(
            install_dir, cls.name, cls._build_dir, cls._targets["debug"]
        )
        release_bin = os.path.join(
            install_dir, cls.name, cls._build_dir, cls._targets["release"]
        )
        if os.path.exists(debug_bin) and (debug or not os.path.exists(release_bin)):
            return debug_bin, True
        if os.path.exists(release_bin):
            return release_bin, False
        assert False, "gem5 binary not found!"

    @classmethod
    def define_options(cls, parser):
        parser.add_argument("target", choices=cls._targets.keys(), help="target binary")

    def _build(self, project_dir, hmv_dir, job_count):
        with chdir(project_dir):
            process = subprocess.run(
                [
                    "scons",
                    "--ignore-style",
                    "EXTRAS={}".format(hmv_dir),
                    os.path.join(self._build_dir, self.target_bin),
                    "-j{}".format(job_count),
                    "--linker=mold",
                ]
            )
        assert process.returncode == 0, "failed to compile gem5"

    def _clone_and_patch(self, hmv_dir):
        project_dir = os.path.join(self.install_dir, self.name)
        _prepare_patch_branch_cmds = [
            ["git", "config", "user.email", "<>"],
            ["git", "config", "user.name", "HOME-V Builder"],
            ["git", "switch", "-c", "hmv-patches"],
        ]
        process = subprocess.run(
            ["git", "clone", "--branch", self.version, self.git_url, project_dir]
        )
        assert process.returncode == 0, "failed to clone gem5's repository"
        with chdir(project_dir):
            for cmd in _prepare_patch_branch_cmds:
                process = subprocess.run(cmd)
                assert (
                    process.returncode == 0
                ), "failed to prepare new branch for patches in gem5"
            for patch in self.patches:
                process = subprocess.run(
                    ["git", "apply", os.path.join(hmv_dir, "integration", patch)]
                )
                assert process.returncode == 0, "failed to apply patch: {}".format(
                    patch
                )

    def _config_build(self, project_dir):
        with chdir(project_dir):
            process = subprocess.run(
                [
                    "scons",
                    "defconfig",
                    "--ignore-style",
                    self._build_dir,
                    "build_opts/ALL",
                ]
            )
            assert (
                process.returncode == 0
            ), "failed to create gem5's build configuration"
            process = subprocess.run(
                [
                    "scons",
                    "setconfig",
                    "--ignore-style",
                    self._build_dir,
                    "RUBY=y",
                    'PROTOCOL="MULTIPLE"',
                    "RUBY_PROTOCOL_MESI_Two_Level=y",
                    "RUBY_PROTOCOL_MESI_Three_Level=y",
                    "RUBY_PROTOCOL_MOESI_CMP_directory=y",
                    "RUBY_PROTOCOL_CHI=n",
                    "USE_SYSTEMC=n",
                ]
            )
            assert (
                process.returncode == 0
            ), "failed to update gem5's build configuration"


__all__ = [Builder, Dispatcher]
