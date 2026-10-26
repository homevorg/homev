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
from importlib import resources
import os
import tomlkit


class Application:
    def __init__(self, config_key, config_file=None, preset=None):
        with resources.as_file(resources.files("hmv.resources")) as path:
            with open(os.path.join(path, "configuration.toml"), "rb") as file:
                all_configs = tomlkit.load(file)
        if config_file:
            with open(config_file, "rb") as file:
                self._merge_configuration(all_configs, tomlkit.load(file))
        if preset:
            all_configs[config_key] = self._merge_configuration(
                all_configs[config_key], all_configs[config_key]["presets"][preset]
            )
        self.configs = all_configs[config_key]

    def _merge_configuration(cls, default, user):
        is_dict = lambda a: isinstance(a, dict)
        for key in user:
            if key in default and is_dict(default[key]) and is_dict(user[key]):
                cls._merge_configuration(default[key], user[key])
            else:
                default[key] = user[key]
        return default

    @classmethod
    def make_parser(cls, name, description):
        parser = ArgumentParser(prog=name, description=description)
        parser.add_argument(
            "-C",
            "--config-file",
            default=None,
            help="load custom configuration file (TOML)",
        )
        return parser

    @classmethod
    def make_subparser(cls, subparsers, Class):
        parser = subparsers.add_parser(Class.name, description=Class.description)
        parser.set_defaults(**{subparsers.dest: Class})
        Class.define_options(parser)
        return parser
