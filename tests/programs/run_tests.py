#!/usr/bin/env python3
import argparse
import pathlib
import sys
import unittest
import subprocess
import tempfile

TESTS_DIRECTORY = pathlib.Path('org').absolute()

binary_dir: str | None = None


def run_javac(file: str, destdir: str):
    javac_command = ['javac', '-source', '8', '-target', '8', '-d', destdir, file]

    print(f"Running {' '.join(javac_command)}")
    result = subprocess.run(javac_command, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    if result.returncode != 0:
        raise RuntimeError("javac failed: " + result.stderr.decode())


class JavaIntegrationTest(unittest.TestCase):
    geevm_java_command: str
    class_dir: tempfile.TemporaryDirectory

    @classmethod
    def setUpClass(cls):
        if binary_dir is None:
            raise ValueError("geevm binary directory must be set!")
        java = pathlib.Path(binary_dir) / 'java'
        if not java.exists():
            raise RuntimeError(f'{java.absolute()} must exist!')

        cls.geevm_java_command = str(java.absolute())

        # Set the class directory
        cls.class_dir = tempfile.TemporaryDirectory(prefix='geevm_java_tests')
        java_files = TESTS_DIRECTORY.glob('./**/*.java')
        for file in java_files:
            run_javac(str(file), cls.class_dir.name)

    @classmethod
    def tearDownClass(cls):
        cls.class_dir.cleanup()

    def run_geevm_java(self, classname: str):
        java_command = [self.geevm_java_command, classname]
        return subprocess.run(java_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=self.class_dir.name)

    def test_simple_math_programs(self):
        tests = [
            ('Count', '10\n'),
            ('StaticCalls', '15\n'),
            ('StaticCallsToAnotherClass', '15\n'),
            ('StaticFields', '540\n')
        ]

        for class_name, expected in tests:
            with self.subTest(class_name):
                r = self.run_geevm_java(f'org.geevm.tests.basic.{class_name}')
                self.assertEqual(expected, r.stdout.decode())


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--geevm-binary-dir', required=True)

    args = parser.parse_args()
    binary_dir = args.geevm_binary_dir
    unittest.main(argv=[sys.argv[0]])
