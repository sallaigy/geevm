#!/usr/bin/env python3
import argparse
import pathlib
import subprocess
import tempfile
import difflib

from typing import List

TESTS_DIRECTORY = pathlib.Path('org').absolute()


class Color:
    RED = '\033[31m'
    GREEN = '\033[32m'
    RESET = '\033[0m'


def run_javac(file: str, destdir: str):
    javac_command = ['javac', '-source', '8', '-target', '8', '-d', destdir, file]

    # print(f"Running {' '.join(javac_command)}")
    result = subprocess.run(javac_command, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    if result.returncode != 0:
        raise RuntimeError("javac failed: " + result.stderr.decode())


class TestCase:
    def __init__(self, name, stdout=None, stderr=None):
        assert (stdout != None or stderr != None)
        self.name = name
        self.expected_stdout = stdout
        self.expected_stderr = stderr


class JavaIntegrationTest:
    geevm_java_command: str
    class_dir: tempfile.TemporaryDirectory
    tests_dir: pathlib.Path

    def __init__(self, geevm_binary_dir: str):
        java_path = pathlib.Path(geevm_binary_dir) / 'java'
        java_path = java_path.absolute()
        if not java_path.exists():
            raise RuntimeError(f'{java_path} must exist')

        self.geevm_java_command = str(java_path)

    def __enter__(self):
        # Set the class and tests directory
        self.class_dir = tempfile.TemporaryDirectory(prefix='geevm_java_tests')
        java_files = TESTS_DIRECTORY.glob('./**/*.java')
        for file in java_files:
            run_javac(str(file), self.class_dir.name)

        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.class_dir.cleanup()

    def run_geevm_java(self, classname: str):
        java_command = [self.geevm_java_command, classname]
        return subprocess.run(java_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=self.class_dir.name,
                              timeout=10)

    def simple_math_programs(self):
        self.execute_tests('simple_math_programs', [
            TestCase('Count', '10\n'),
            TestCase('StaticCalls', '15\n'),
            TestCase('StaticCallsToAnotherClass', '15\n'),
            TestCase('StaticFields', '540\n'),
            TestCase('IntegerComparisons', '0\n3\n5\n1\n2\n5\n2\n3\n4\n'),
            TestCase('IntegerCompareZero', '1\n2\n5\n2\n3\n4\n0\n3\n5\n'),
            TestCase('StaticFieldsLong', '540\n')
        ])

    def oop_programs(self):
        self.execute_tests('oop_programs', [
            TestCase('Instance', '42\n'),
            TestCase('Inheritance', '10\n20\n10\n40\n'),
            TestCase('InheritanceFields', '10\n10\n10\n40\n')
        ])

    def strings(self):
        self.execute_tests('strings', [
            TestCase('strings.HelloWorld', 'Hello World!\n'),
            TestCase('strings.StringEquals', 'true\nfalse\nfalse\ntrue\ntrue\nfalse\ntrue\nfalse\n')
        ])

    def exceptions(self):
        self.execute_tests('exceptions', [
            TestCase('exceptions.SimpleException',
                     stdout='Caught exception: Exception thrown and caught in the same method.\n'),
            TestCase('exceptions.ExceptionInCallee',
                     stdout='Caught exception: Exception thrown in callee.\n'),
            TestCase('exceptions.UncaughtException',
                     stderr="Exception java.lang.IllegalStateException: 'Exception thrown in callee.'\n")
        ])

    def run(self):
        self.simple_math_programs()
        self.strings()
        self.oop_programs()
        self.exceptions()

    def execute_tests(self, test_suite_name, tests: List[TestCase]):
        success: bool = True
        print(test_suite_name)
        for test_case in tests:
            class_name = test_case.name
            expected_stdout = test_case.expected_stdout
            expected_stderr = test_case.expected_stderr

            try:
                r = self.run_geevm_java(f'org.geevm.tests.basic.{class_name}')
                if r.returncode != 0:
                    print(f'  [{Color.RED}ERROR{Color.RESET}] {class_name}')
                    print(
                        f'  {Color.RED}The geevm java command failed with status code {r.returncode}{Color.RESET}:')
                    print(r.stderr.decode())
                    continue

                if expected_stdout is not None:
                    actual = r.stdout.decode()
                    success = self.compare(class_name, 'stdout', expected_stdout, actual) and success
                if expected_stderr is not None:
                    actual = r.stderr.decode()
                    success = self.compare(class_name, 'stderr', expected_stderr, actual) and success
            except TimeoutError:
                print(f'  [{Color.RED}TIMEOUT{Color.RESET}] {class_name}')
                print(
                    f'  {Color.RED}The geevm java failed due to timeout {Color.RESET}:')
                continue

    def compare(self, name, comparing, expected, actual) -> bool:
        if expected != actual:
            print(f'  [{Color.RED}FAIL{Color.RESET}] {name}')
            print(f'actual {comparing} is different than expected')
            print(''.join(difflib.unified_diff(expected, actual, fromfile='expected', tofile='actual')))
            return False
        else:
            print(f'  [{Color.GREEN}PASS{Color.RESET}] {name}')
            return True


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--geevm-binary-dir', required=True)

    args = parser.parse_args()
    binary_dir = args.geevm_binary_dir

    with JavaIntegrationTest(binary_dir) as test:
        test.run()
