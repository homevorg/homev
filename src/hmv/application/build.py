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

from .common import Application
from hmv.platform import gem5


class Builder(Application):
    _default_name = "hmv-build"
    _default_desc = "Centralized builder for platforms"
    _platforms = [gem5.Builder]
    _config_key = "builder"

    def __init__(self, config_file, job_count, Platform, **kwargs):
        super().__init__(self._config_key, config_file)
        self.job_count = job_count
        self.builder = Platform(self.configs, self.configs["install_dir"], **kwargs)

    def __call__(self):
        self.builder(self.job_count)

    @classmethod
    def make_parser(cls, name=_default_name, description=_default_desc):
        parser = super().make_parser(name, description)
        parser.add_argument(
            "-j", "--job-count", type=int, default=1, help="number of jobs"
        )
        platforms = parser.add_subparsers(dest="Platform", required=True)
        for Platform in cls._platforms:
            cls.make_subparser(platforms, Platform)
        return parser


def main():
    parser = Builder.make_parser()
    args = parser.parse_args()
    Builder(**vars(args))()


if __name__ == "__main__":
    main()
