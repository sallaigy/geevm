#!/usr/bin/env python3
import argparse
import pathlib
import subprocess
import tempfile
import difflib

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
        return subprocess.run(java_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=self.class_dir.name)

    def simple_math_programs(self):
        tests = [
            ('Count', '10\n'),
            ('StaticCalls', '15\n'),
            ('StaticCallsToAnotherClass', '15\n'),
            ('StaticFields', '540\n')
        ]

        for class_name, expected in tests:
            r = self.run_geevm_java(f'org.geevm.tests.basic.{class_name}')
            if r.returncode != 0:
                print(f'[{Color.RED}ERROR{Color.RESET}] {class_name}')
                print(f'{Color.RED}The geevm java command failed with status code {r.returncode}{Color.RESET}:')
                print(r.stderr.decode())
                continue

            actual = r.stdout.decode()
            if expected != actual:
                print(f'[{Color.RED}FAIL{Color.RESET}] {class_name}')
                print(''.join(difflib.unified_diff(expected, actual, fromfile='expected', tofile='actual')))
            else:
                print(f'[{Color.GREEN}PASS{Color.RESET}] {class_name}')

    def run(self):
        self.simple_math_programs()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--geevm-binary-dir', required=True)

    args = parser.parse_args()
    binary_dir = args.geevm_binary_dir

    with JavaIntegrationTest(binary_dir) as test:
        test.run()
