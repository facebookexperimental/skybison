#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
"""
Utility script that provides default arguments for executing a command
with various performance measurement tools.
"""
import logging
import os
import re
import subprocess
import tempfile
from abc import ABC, abstractmethod
from multiprocessing.pool import ThreadPool


log = logging.getLogger(__name__)
SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))


def run(cmd, **kwargs):
    env = dict(os.environ)
    env["PYTHONHASHSEED"] = "0"
    log.info(f">>> {' '.join(cmd)}")
    return subprocess.run(cmd, encoding="UTF-8", env=env, check=True, **kwargs)


def create_taskset_command(isolated_cpus):
    if isolated_cpus == "":
        return []
    # If it ever matters in the future, this only pulls out the first integer
    # encountered in the list of isolated cpus
    isolated_cpus = re.findall(r"\d+", isolated_cpus)[0]
    return ["taskset", "--cpu-list", isolated_cpus]


def pin_to_cpus():
    if not os.path.exists("/sys/devices/system/cpu/isolated"):
        return []
    completed_process = run(
        ["cat", "/sys/devices/system/cpu/isolated"], stdout=subprocess.PIPE
    )
    isolated_cpus = completed_process.stdout.strip()
    return create_taskset_command(isolated_cpus)


class PerformanceTool(ABC):
    # Read any optional command line arguments to set the internal defaults
    # Input: A dictionary with command line arguments
    def __init__(self, args):
        pass

    # Specify the name of the tool along with a description
    @staticmethod
    @abstractmethod
    def add_tool():
        return ""

    # Add any optional command line arguments to tune the tool
    @staticmethod
    def add_optional_arguments(parser):
        return parser


class SequentialPerformanceTool(PerformanceTool):
    # The main function to execute the specified performance tool.
    # Input: run.Interpreter, run.Benchmark
    # Output: A dictionary with the values to be reported
    @abstractmethod
    def execute(self, interpreter, benchmark):
        pass


class ParallelPerformanceTool(PerformanceTool):
    # TODO update
    # The main function to execute the specified performance tool in parallel.
    # Input: list<run.Interpreter>, list<run.Benchmark>
    # Output: A list of dictionaries with the values to be reported. Each
    #         dictionary must have both 'benchmark' and 'interpreter' reported
    @abstractmethod
    def execute_parallel(self, interpreters, benchmarks):
        pass


class TimeTool(SequentialPerformanceTool):
    NAME = "time"

    def execute(self, interpreter, benchmark):
        command = pin_to_cpus()
        command.extend(
            [
                *interpreter.interpreter_cmd,
                f"{SCRIPT_DIR}/_time_tool.py",
                # The time tool imports the module, which will use the bytecode
                # cache. Pass the source file instead of the bytecode file.
                benchmark.filepath(),
                *interpreter.benchmark_args,
            ]
        )
        completed_process = run(command, stdout=subprocess.PIPE)
        time_output = completed_process.stdout.strip()
        events = [event.split(" , ") for event in time_output.split("\n")]
        result = {event[0]: event[1] for event in events}
        if "time_sec" in result:
            result["time_sec"] = float(result["time_sec"])
        if "time_sec_mean" in result:
            result["time_sec_mean"] = float(result["time_sec_mean"])
            result["time_sec_stdev"] = float(result["time_sec_stdev"])
        return result

    @staticmethod
    def add_tool():
        return f"""
'{TimeTool.NAME}': Use the 'time' command to measure execution time
"""


class PerfStat(SequentialPerformanceTool):
    NAME = "perfstat"
    DEFAULT_EVENTS = ["task-clock", "instructions"]

    def __init__(self, args):
        self.events = PerfStat.DEFAULT_EVENTS if not args["events"] else args["events"]

    def parse_perfstat(self, output):
        if ";" not in output:
            log.error(f"perf stat returned an error: {output}")
            return {}
        events = [e.split(";") for e in output.split("\n") if ";" in e]
        results = {}
        for event in events:
            name = event[2]
            value = event[0]
            if value in ("<not counted>", "<not supported>", ""):
                continue
            value = float(value) if "." in value else int(value)
            results[name] = value
        return results

    def execute(self, interpreter, benchmark):
        command = pin_to_cpus()
        command += ["perf", "stat"]
        command += ["--field-separator", ";"]
        command += ["--repeat", "5"]

        # To avoid event multiplexing, we only run two events at a time
        results = {}
        events = [event for event in self.events]
        bytecode_path = compile_bytecode(interpreter, benchmark)
        while events:
            full_command = command + ["--event", events.pop(0)]
            if events:
                full_command += ["--event", events.pop(0)]
            full_command += [
                *interpreter.interpreter_cmd,
                bytecode_path,
                *interpreter.benchmark_args,
            ]
            completed_process = run(full_command, stderr=subprocess.PIPE)
            perfstat_output = completed_process.stderr.strip()
            results.update(self.parse_perfstat(perfstat_output))
        return results

    @staticmethod
    def add_tool():
        return f"""
'{PerfStat.NAME}': Use `perf stat` to measure the execution time of
a benchmark. This repeats the run 10 times to find a significant result
"""

    # Add any optional command line arguments to tune the tool
    @staticmethod
    def add_optional_arguments(parser):
        perfstat_event_help = f"""
Specify the perf stat event to run. Please note, only two are run at the
same time to avoid event multiplexing. For a full list of perf stat events,
run: `perf list`.

Examples: 'instructions', 'branch-misses', 'L1-icache-load-misses'

Default: {PerfStat.DEFAULT_EVENTS}
"""
        parser.add_argument(
            "--event",
            metavar="EVENT",
            dest="events",
            type=str,
            action="append",
            default=[],
            help=perfstat_event_help,
        )
        return parser


class Callgrind(ParallelPerformanceTool):
    NAME = "callgrind"

    def __init__(self, args):
        self.callgrind_out_dir = args.get("callgrind_out_dir")

    def _worker(self, interpreter, benchmark):
        delete = True
        callgrind_out_dir = self.callgrind_out_dir
        if callgrind_out_dir is not None:
            callgrind_out_dir = os.path.abspath(callgrind_out_dir)
            os.makedirs(callgrind_out_dir, exist_ok=True)
            delete = False
        with tempfile.NamedTemporaryFile(
            dir=callgrind_out_dir,
            prefix=f"{benchmark.name}_",
            suffix=".cg",
            delete=delete,
        ) as temp_file:
            bytecode_path = compile_bytecode(interpreter, benchmark)
            run(
                [
                    "valgrind",
                    "--quiet",
                    "--tool=callgrind",
                    "--trace-children=yes",
                    f"--callgrind-out-file={temp_file.name}",
                    *interpreter.interpreter_cmd,
                    bytecode_path,
                    *interpreter.benchmark_args,
                ]
            )

            instructions = 1
            with open(temp_file.name) as fd:
                r = re.compile(r"summary:\s*(.*)")
                for line in fd:
                    m = r.match(line)
                    if m:
                        instructions = int(m.group(1))
            return {
                "benchmark": benchmark.name,
                "interpreter": interpreter.name,
                "cg_instructions": instructions,
            }

    def execute_parallel(self, interpreters, benchmarks):
        pool = ThreadPool()
        async_results = []
        for interpreter in interpreters:
            for benchmark in benchmarks:
                r = pool.apply_async(self._worker, (interpreter, benchmark))
                async_results.append(r)

        results = []
        for ar in async_results:
            results.append(ar.get())
        return results

    @classmethod
    def add_tool(cls):
        return f"""
'{cls.NAME}': Measure executed instructions with `valgrind`/`callgrind`.
"""

    @staticmethod
    def add_optional_arguments(parser):
        parser.add_argument("--callgrind-out-dir", metavar="DIRECTORY")
        return parser


class Size(SequentialPerformanceTool):
    NAME = "size"

    def __init__(self, args):
        pass

    def execute(self, interpreter, benchmark):
        command = ["size", "--format=sysv", interpreter.binary]
        completed_process = run(command, stdout=subprocess.PIPE)
        size_output = completed_process.stdout.strip()
        size = 0
        r = re.compile(r"([a-zA-Z0-9_.]+)\s+([0-9]+)\s+[0-9a-fA-F]+$")
        for line in size_output.splitlines():
            m = r.match(line)
            if not m:
                continue
            section_name = m.group(1)
            section_size = m.group(2)
            if section_name == ".text" or section_name == "__text":
                size += int(section_size)
        if size == 0:
            log.error(f"Could not determine text segment size of {interpreter.binary}")
            return {}
        return {"size_text": size}

    @classmethod
    def add_tool(cls):
        return f"""
'{cls.NAME}': Use `size` to measure the size of the interpreters text segment.
"""


def add_tools_arguments(parser):
    measure_tools_help = "The measurement tool to use. Available Tools: \n"
    for tool in TOOLS:
        measure_tools_help += tool.add_tool()

    available_tools = [tool.NAME for tool in TOOLS]
    parser.add_argument(
        "--tool",
        "-t",
        metavar="TOOL",
        dest="tools",
        type=str,
        action="append",
        default=[],
        choices=available_tools,
        help=measure_tools_help,
    )

    for tool in TOOLS:
        parser = tool.add_optional_arguments(parser)

    return parser


def compile_bytecode(interpreter, benchmark):
    log.info(f"Compiling benchmark for {interpreter.name}: {benchmark.name}")
    command = [
        *interpreter.interpreter_cmd,
        f"{SCRIPT_DIR}/_compile_tool.py",
        benchmark.filepath(),
        *interpreter.benchmark_args,
    ]
    result = run(command, stdout=subprocess.PIPE)
    return result.stdout.lstrip().rstrip()  # remove '\n'


# Use this to register any new tools
SEQUENTIAL_TOOLS = [TimeTool, PerfStat, Size]
PARALLEL_TOOLS = [Callgrind]
TOOLS = SEQUENTIAL_TOOLS + PARALLEL_TOOLS
