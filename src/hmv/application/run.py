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

from argparse import ArgumentParser
from contextlib import chdir
from functools import reduce
from math import log2
from random import Random
import csv
import os
import shutil
import tomlkit

from .common import Application
from hmv import (
    ISA,
    MemoryTraits,
    Monitor,
    Operation,
    TimeCounter,
    TraceSelection,
    WordSize,
)
from hmv.checker import Type as CheckerType
from hmv.coverage import Type as CoverageType
from hmv.generator import htg, mcversi, random, sca, udt
from hmv.platform import gem5


class Runner(Application):
    name = "hmv-run"
    description = "Centralized runner for generators and checkers"
    configuration_key = "runner"
    _checkers = [c.name for c in CheckerType]
    _metrics = [c.name for c in CoverageType]
    _architectures = [i.name for i in ISA]
    _monitors = [m.name for m in Monitor if m.name != "NONE"] + ["ALL"]
    _generators = [
        htg.Generator,
        mcversi.Generator,
        random.RnB,
        random.RnR,
        random.CnB,
        random.CnR,
        sca.Generator,
        udt.Generator,
    ]
    _dispatchers = [gem5.Dispatcher]
    _crash_filename = "PLATFORM_CRASHED.TXT"
    _parameters_filename = "parameters.toml"
    _results_filename = "tests.csv"
    _trace_filename = "traces/{}.txt"

    def __init__(
        self,
        config_file,
        checkers,
        metrics,
        architecture,
        preset,
        processors,
        warmup_instances,
        test_limit,
        time_limit,
        halt_on_coverage,
        run_count,
        master_seed,
        start_from,
        trace_all,
        trace_read,
        trace_write,
        trace_fence,
        overwrite,
        details,
        quiet,
        Generator,
        Dispatcher,
        **kwargs,
    ):
        super().__init__(self.configuration_key, config_file, preset)
        self.checkers = [CheckerType[c] for c in checkers]
        self.metrics = [CoverageType[m] for m in metrics]
        self.architecture = ISA[architecture]
        self.preset = preset
        self.memory_traits = MemoryTraits(
            self.configs["address_size"], WordSize[self.configs["word_size"]]
        )
        self.processors = processors
        self.warmup_instances = warmup_instances
        self.test_limit = test_limit
        self.time_limit = time_limit * 1000000000
        self.halt_on_coverage = halt_on_coverage
        self.run_count = run_count
        self.master_seed = master_seed
        self.start_from = start_from
        self.overwrite = overwrite
        self.details = details
        self.quiet = quiet
        self.install_dir = self.configs["install_dir"]
        self.output_dir = self.configs["output_dir"]
        self.user_trace_selection = self._make_trace_selection(
            trace_all, trace_read, trace_write, trace_fence
        )
        self.dispatcher = Dispatcher(
            self.install_dir,
            self.memory_traits,
            self.architecture,
            self.processors,
            self.warmup_instances,
            self.checkers,
            self.metrics,
            self.details,
            self.configs.get(Dispatcher.configuration_key, dict()).unwrap(),
            **kwargs,
        )
        self.generator = Generator(
            self.memory_traits,
            self.dispatcher.get_last_level_cache_traits(),
            self.processors,
            self.metrics,
            self.configs.get(Generator.configuration_key, dict()).unwrap(),
            **kwargs,
        )
        self.dispatcher.set_max_address_count(self.generator.max_address_count())
        self.dispatcher.set_max_thread_size(self.generator.max_thread_size())
        self.dispatcher.set_trace_selection(self._get_merged_trace_selection())
        self.main_time_counter = TimeCounter()
        self.generator_time_counter = TimeCounter()
        self._reset_stats()

    def __call__(self):
        urng = Random(self.master_seed)
        for _ in range(self.start_from):
            urng.randint(0, (1 << 64) - 1)
        for _ in range(self.run_count):
            self.seed = urng.randint(0, (1 << 64) - 1)
            if result_dir := self._make_result_dir():
                self._execute(result_dir)

    def _deploy(self):
        print("> Initializing seed", self.seed)
        self.generator_time_counter.reset()
        self.main_time_counter.reset()
        self.main_time_counter.start()
        self.generator.set_seed(self.seed)
        self.dispatcher.deploy(self.seed)
        self.main_time_counter.stop()
        print(
            "> Initialization done after {:.2f}s".format(
                self.main_time_counter.seconds()
            )
        )
        self.main_time_counter.reset()
        self.main_time_counter.start()

    def _destroy(self):
        self.dispatcher.destroy()
        self.tests_file.close()
        self._reset_stats()

    def _execute(self, result_dir):
        with chdir(result_dir):
            self._deploy()
            while test := self._next_test():
                self._display_test()
                returncode, output = self.dispatcher.dispatch(test)
                stop = self._feedback(returncode, output)
                self._save_results(returncode, output)
                self._display_results(returncode, output)
                if stop or self._stop(output):
                    break
            for m in output.aggregated_metrics:
                m.write(m.type().name + ".dat")
                m.write_csv(m.type().name + ".csv")
            self._destroy()

    def _next_test(self):
        self.generator_time_counter.start()
        test = self.generator()
        self.generator_time_counter.stop()
        return test

    def _feedback(self, returncode, output):
        if returncode != 0:
            return True
        self.generator_time_counter.start()
        stop = self.generator.feedback(output)
        self.generator_time_counter.stop()
        return stop

    def _make_result_dir(self):
        result_dir = os.path.join(
            self.output_dir,
            "{}.{}.{}".format(
                self._unique_id(),
                self.generator.unique_id(),
                self.dispatcher.unique_id(),
            ),
            str(self.seed),
        )
        if os.path.exists(result_dir):
            if not self.overwrite:
                self._display_skip()
                return None
            shutil.rmtree(result_dir)
        os.makedirs(result_dir, exist_ok=True)
        if self.details:
            for m in self.metrics:
                os.makedirs(os.path.join(result_dir, m.name), exist_ok=True)
        return result_dir

    def _fields(self):
        fields = ["test_id"] + self.generator.fields()
        for c in self.checkers:
            fields.append(c.name)
            fields.append(c.name + "_time")
        for m in self.metrics:
            fields.append(m.name)
            fields.append(m.name + "_aggregated")
        fields += ["generator_time", "warmup_time", "test_time", "total_time"]
        return fields

    def _field_values(self, output):
        field_values = {
            "test_id": self.test_id,
            "generator_time": self.generator_time,
            "warmup_time": output.warmup_time,
            "test_time": output.test_time,
            "total_time": self.total_time,
        } | self.generator.field_values()
        for c in output.checkers:
            field_values[c.type.name] = c.code
            field_values[c.type.name + "_time"] = c.time
        for m in output.metrics:
            field_values[m.type().name] = m.value()
        for m in output.aggregated_metrics:
            field_values[m.type().name + "_aggregated"] = m.value()
        return field_values

    def _parameters(self):
        return (
            {
                "checkers": [c.name for c in self.checkers],
                "metrics": [m.name for m in self.metrics],
                "architecture": self.architecture.name,
                "preset": self.preset,
                "processors": self.processors,
                "warmup_instances": self.warmup_instances,
                "generator": self.generator.name,
                "platform": self.dispatcher.name,
                "seed": self.seed,
            }
            | self.generator.parameters()
            | self.dispatcher.parameters()
        )

    def _display_results(self, returncode, output):
        if not self.quiet:
            self.main_time_counter.stop()
            print(
                "    generator time: {:.2f}ms, test time: {:.2f}s [total: {:.2f}s]".format(
                    self.generator_time / 1000000,
                    output.test_time / 1000000000,
                    self.total_time / 1000000000,
                )
            )
            for i in range(len(output.metrics)):
                print(
                    "    {}: {:.2f}% [aggregated: {:.2f}%]".format(
                        output.metrics[i].type().name,
                        output.metrics[i].percentage(),
                        output.aggregated_metrics[i].percentage(),
                    )
                )
            for c in output.checkers:
                print(
                    "    {}: {} [time: {:.2f}ms]".format(
                        c.type.name, c.code, c.time / 1000000
                    )
                )
            self.main_time_counter.start()
        if returncode != 0:
            print("> Platform crashed with code", returncode)

    def _display_skip(self):
        print(
            "> Results for ",
            self.seed,
            "found! Skipping... (use -w/--overwrite to rerun)",
        )

    def _display_test(self):
        if not self.quiet:
            self.main_time_counter.stop()
            print(
                "> Running {}({})".format(
                    self.generator.name, self.generator.test_display(), self.seed
                )
            )
            self.main_time_counter.start()

    def _reset_stats(self):
        self.test_id = 0
        self.total_time = 0
        self.generator_time = 0
        self.tests_file = None
        self.tests_writer = None

    def _save_results(self, returncode, output):
        if self.test_id == 0:
            self._write_parameters(output)
        self.test_id += 1
        self.total_time = self.main_time_counter.nanoseconds()
        self.generator_time = self.generator_time_counter.nanoseconds()
        if not self.tests_writer:
            self.tests_file = open(self._results_filename, "w")
            self.tests_writer = csv.DictWriter(self.tests_file, self._fields())
            self.tests_writer.writeheader()
        self.tests_writer.writerow(self._field_values(output))
        if self.details:
            for m in output.metrics:
                m.write(os.path.join(m.type().name, str(self.test_id) + ".dat"))
        if self.user_trace_selection:
            output.trace.filter(self.user_trace_selection).write_log(
                os.path.join(self._trace_filename.format(str(self.test_id))),
                self.memory_traits,
            )
        if returncode != 0:
            with open(self._crash_filename, "w") as file:
                file.write("return code: " + str(returncode))

    def _stop(self, output):
        or_fn = lambda x, y: x or y
        return (
            (self.test_limit != 0 and self.test_id >= self.test_limit)
            or (self.time_limit != 0 and self.total_time >= self.time_limit)
            or (
                self.halt_on_coverage
                and reduce(or_fn, [m.full() for m in output.aggregated_metrics])
            )
            or reduce(or_fn, [c.code != 0 for c in output.checkers])
        )

    def _unique_id(self):
        return "{}.{}.{}.{}.p{}.w{}".format(
            ".".join([c.name for c in self.checkers]),
            ".".join([m.name for m in self.metrics]),
            self.architecture.name,
            self.preset,
            self.processors,
            self.warmup_instances,
        )

    def _write_parameters(self, output):
        with open(self._parameters_filename, "w") as file:
            params = self._parameters()
            for m in output.metrics:
                params[m.type().name] = {"max": m.max()}
            file.write(tomlkit.dumps(params))

    def _get_merged_trace_selection(self):
        trace_selection = self.generator.trace_selection()
        for op, selection in self.user_trace_selection.items():
            if op.name in trace_selection:
                trace_selection[op] = trace_selection[op] | selection
            else:
                trace_selection[op] = selection
        return trace_selection

    def _make_trace_selection(self, trace_all, trace_read, trace_write, trace_fence):
        translate = lambda options: (
            {m for m in Monitor if m.name != "NONE"}
            if "ALL" in options
            else {Monitor[m] for m in options}
        )
        trace_selection = TraceSelection()
        all_selection = translate(trace_all)
        selections = {
            Operation.Type.LOAD: translate(trace_read),
            Operation.Type.STORE: translate(trace_write),
            Operation.Type.FENCE: translate(trace_fence),
        }
        for op, selection in selections.items():
            if all_selection:
                selection.update(all_selection)
            if selection:
                if op.name in trace_selection:
                    selection.update(trace_selection[op.name])
                trace_selection[op] = selection
        return trace_selection

    @classmethod
    def make_parser(cls, name=name, description=description):
        parser = super().make_parser(name, description)
        parser.add_argument(
            "-c",
            "--checkers",
            required=True,
            action="append",
            choices=cls._checkers,
            help="memory consistency model checkers",
        )
        parser.add_argument(
            "-m",
            "--metrics",
            "--coverage-metrics",
            required=True,
            action="append",
            choices=cls._metrics,
            help="coverage metrics",
        )
        parser.add_argument(
            "-A",
            "--architecture",
            required=True,
            choices=cls._architectures,
            help="target instruction set architecture (ISA)",
        )
        parser.add_argument(
            "-P",
            "--preset",
            required=True,
            help="preset to use from the configuration file (MESI2, MESI3, MOESI, ...)",
        )
        parser.add_argument(
            "-p",
            "--processors",
            type=int,
            default=8,
            help="number of processors (cores)",
        )
        parser.add_argument(
            "-w",
            "--warmup-instances",
            type=int,
            default=1,
            help="number of warmup execution instances",
        )
        parser.add_argument(
            "-l",
            "--test-limit",
            type=int,
            default=0,
            help="set a test limit for the run (0 = unlimited)",
        )
        parser.add_argument(
            "-L",
            "--time-limit",
            type=int,
            default=0,
            help="set a time limit (in seconds) for the run (0 = unlimited)",
        )
        parser.add_argument(
            "-H",
            "--halt-on-coverage",
            action="store_true",
            help="halt run if maximum total coverage is reached",
        )
        parser.add_argument(
            "-r",
            "--run-count",
            type=int,
            default=1,
            help="number of times to run (each with a different seed)",
        )
        parser.add_argument(
            "-S",
            "--master-seed",
            type=int,
            default=13691253077952742952,
            help="master seed to generate each run seed",
        )
        parser.add_argument(
            "-s",
            "--start-from",
            type=int,
            default=0,
            help="start from the specified position in the seed sequence",
        )
        parser.add_argument(
            "--trace-all",
            nargs="+",
            choices=cls._monitors,
            default=[],
            help="trace all events for the given monitor locations",
        )
        parser.add_argument(
            "--trace-read",
            nargs="+",
            choices=cls._monitors,
            default=[],
            help="trace read events for the given monitor locations",
        )
        parser.add_argument(
            "--trace-write",
            nargs="+",
            choices=cls._monitors,
            default=[],
            help="trace write events for the given monitor locations",
        )
        parser.add_argument(
            "--trace-fence",
            nargs="+",
            choices=cls._monitors,
            default=[],
            help="trace fence events for the given monitor locations",
        )
        parser.add_argument(
            "-o",
            "--overwrite",
            action="store_true",
            help="overwrite existing results",
        )
        parser.add_argument(
            "-d",
            "--details",
            action="store_true",
            help="enable detailed results",
        )
        parser.add_argument(
            "-q",
            "--quiet",
            action="store_true",
            help="don't write to stdout during run",
        )
        generators = parser.add_subparsers(
            prog="generator", dest="Generator", required=True
        )
        for Generator in cls._generators:
            g_parser = cls.make_subparser(generators, Generator)
            dispatchers = g_parser.add_subparsers(
                prog="platform", dest="Dispatcher", required=True
            )
            for Dispatcher in cls._dispatchers:
                cls.make_subparser(dispatchers, Dispatcher)
        return parser


def main():
    parser = Runner.make_parser()
    args = parser.parse_args()
    Runner(**vars(args))()


if __name__ == "__main__":
    main()
