#!/usr/bin/env python3
import argparse
import difflib
import pathlib
import subprocess
import tempfile
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
    failures = List[TestCase]

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
            TestCase('basic.Count', '10\n'),
            TestCase('basic.StaticCalls', '15\n'),
            TestCase('basic.IntegerArithmetic',
                     '13\n-2147483648\n7\n2147483647\n30\n3\n-3\n-3\n3\n1\n1\n-1\n-1\n-10\n10\n0\n-2147483648\n-2147483647\n'),
            TestCase('basic.LongArithmetic',
                     '13\n-9223372036854775808\n7\n9223372036854775807\n30\n3\n-3\n-3\n3\n1\n1\n-1\n-1\n-10\n10\n0\n-9223372036854775808\n-9223372036854775807\n'),
            # The format between System.out.println and the debug printer is different.
            # This will have to be adjusted once we support System.out
            TestCase('basic.FloatArithmetic', '''12.75
nan
nan
-nan
inf
-inf
0
0
-0
2.25
2.25
0
nan
8.25
23.625
-23.625
-23.625
23.625
nan
inf
-inf
-inf
inf
inf
-inf
inf
inf
-nan
-nan
4.66667
nan
nan
-nan
-0
0
inf
-inf
1.5
1.5
-1.5
-1.5
-10.5
10.5
-0
0
-1.4013e-45
-3.40282e+38
-nan
inf
-inf
'''),
            TestCase('basic.DoubleArithmetic', '''12.75
nan
nan
-nan
inf
-inf
0
0
-0
2.25
2.25
0
nan
8.25
23.625
-23.625
-23.625
23.625
nan
inf
-inf
-inf
inf
inf
-inf
inf
inf
-nan
-nan
4.66667
nan
nan
-nan
-0
0
inf
-inf
1.5
1.5
-1.5
-1.5
-10.5
10.5
-0
0
-4.94066e-324
-1.79769e+308
-nan
inf
-inf
'''),
            TestCase('basic.StaticCallsToAnotherClass', '15\n'),
            TestCase('basic.StaticFields', '540\n'),
            TestCase('basic.IntegerComparisons', '0\n3\n5\n1\n2\n5\n2\n3\n4\n'),
            TestCase('basic.IntegerCompareZero', '1\n2\n5\n2\n3\n4\n0\n3\n5\n'),
            TestCase('basic.StaticCallsLong', '15\n'),
            TestCase('basic.StaticFieldsLong', '540\n')
        ])

    def oop_programs(self):
        self.execute_tests('oop_programs', [
            TestCase('oop.Instance', '42\n'),
            TestCase('oop.Inheritance', '10\n20\n10\n40\n'),
            TestCase('oop.InheritanceFields', '10\n10\n10\n40\n')
        ])

    def strings(self):
        self.execute_tests('strings', [
            TestCase('strings.HelloWorld', 'Hello World!\n'),
            TestCase('strings.StringEquals', 'true\nfalse\nfalse\ntrue\ntrue\nfalse\ntrue\nfalse\n')
        ])

    def arrays(self):
        expected_number_sequence = '0\n2\n4\n6\n8\n10\n12\n14\n16\n18\n'
        expected_exceptions_for_primitives = '''Caught NegativeArraySizeException
Caught NullPointerException
Caught ArrayIndexOutOfBoundsException
Caught ArrayIndexOutOfBoundsException
Caught ArrayIndexOutOfBoundsException
Caught NullPointerException
Caught ArrayIndexOutOfBoundsException
Caught ArrayIndexOutOfBoundsException
Caught ArrayIndexOutOfBoundsException
'''

        self.execute_tests('arrays', [
            TestCase('arrays.IntArrays', expected_number_sequence),
            TestCase('arrays.IntArrayExceptions', expected_exceptions_for_primitives),
            TestCase('arrays.LongArrays', expected_number_sequence),
            TestCase('arrays.ByteArrays', expected_number_sequence),
            TestCase('arrays.ShortArrays', expected_number_sequence),
            TestCase('arrays.FloatArrays', '0\n1.5\n3\n4.5\n6\n7.5\n9\n10.5\n12\n13.5\n'),
            TestCase('arrays.DoubleArrays', '0\n1.5\n3\n4.5\n6\n7.5\n9\n10.5\n12\n13.5\n'),
            TestCase('arrays.CharArrays', '0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n'),
            TestCase('arrays.CharArrayExceptions', expected_exceptions_for_primitives),
            TestCase('arrays.ObjectArrays', '#0\n#1\n#2\n#3\n#4\n#5\n#6\n#7\n#8\n#9\n'),
            TestCase('arrays.ObjectArrayExceptions', '''Caught NegativeArraySizeException
Caught NullPointerException
Caught ArrayIndexOutOfBoundsException
Caught ArrayIndexOutOfBoundsException
Caught ArrayIndexOutOfBoundsException
Caught ArrayStoreException
Caught NullPointerException
Caught ArrayIndexOutOfBoundsException
Caught ArrayIndexOutOfBoundsException
Caught ArrayIndexOutOfBoundsException
''')
        ])

    def exceptions(self):
        self.execute_tests('exceptions', [
            TestCase('exceptions.SimpleException',
                     stdout='Caught exception: Exception thrown and caught in the same method.\n'),
            TestCase('exceptions.ExceptionInCallee',
                     stdout='Caught exception: Exception thrown in callee.\n'),
            TestCase('exceptions.UncaughtException', stderr='''Exception java.lang.IllegalStateException: 'Exception thrown in callee.'
  at org.geevm.tests.exceptions.UncaughtException.callee(Unknown Source)
  at org.geevm.tests.exceptions.UncaughtException.main(Unknown Source)

''')
        ])

    def errors(self):
        self.execute_tests('errors', [
            TestCase('errors.UnknownNativeMethod',
                     stderr='''Exception java.lang.UnsatisfiedLinkError: 'void org.geevm.tests.errors.UnknownNativeMethod.callee()'
  at org.geevm.tests.errors.UnknownNativeMethod.callee(Unknown Source)
  at org.geevm.tests.errors.UnknownNativeMethod.main(Unknown Source)

'''),
            TestCase('errors.ClassCast',
                     stderr='''Exception java.lang.ClassCastException: 'class org.geevm.tests.errors.ClassCast cannot be cast to class java.lang.String'
  at org.geevm.tests.errors.ClassCast.main(Unknown Source)

''')
        ])

    def reflection(self):
        self.execute_tests('reflection', [
            TestCase('reflection.ClassMetadata',
                     stdout='org.geevm.tests.reflection.ClassMetadata\norg.geevm.tests.reflection.ClassMetadata\norg.geevm.tests.reflection.ClassMetadata\n')
        ])

    def init(self):
        self.execute_tests('init', [
            TestCase('init.ex1.Test',
                     stdout='Super\nTwo\nfalse\n'),
            TestCase('init.ex2.Test',
                     stdout='1729\n'),
            TestCase('init.ex3.Test',
                     stdout='1\nj=3\njj=4\n3\n')
        ])

    def casting(self):
        self.execute_tests('casting', [
            TestCase('casting.IntToByte', stdout='100\n-126\n126\n'),
            TestCase('casting.IntToShort', stdout='100\n-30536\n30536\n'),
            TestCase('casting.IntToChar', stdout='A\nB\nﾾ\n'),
            TestCase('casting.IntToLong', stdout='100\n-2147483648\n2147483647\n'),
            TestCase('casting.IntToFloat',
                     stdout='12345\n-12345\n1.67772e+07\n1.67772e+07\n2.14748e+09\n-2.14748e+09\n'),
            TestCase('casting.IntToDouble',
                     stdout='12345\n-12345\n1.67772e+07\n1.67772e+07\n2.14748e+09\n-2.14748e+09\n'),
            TestCase('casting.LongToInt',
                     stdout='100\n2147483647\n-2147483648\n0\n-1\n'),
            TestCase('casting.LongToFloat',
                     stdout='12345\n-12345\n-2.14748e+09\n2.14748e+09\n-9.22337e+18\n9.22337e+18\n'),
            TestCase('casting.LongToDouble',
                     stdout='12345\n-12345\n-2.14748e+09\n2.14748e+09\n-9.22337e+18\n9.22337e+18\n'),
            TestCase('casting.FloatToInt',
                     stdout='42\n-42\n0\n2147483647\n0\n-2147483648\n2147483647\n-2147483648\n0\n2147483647\n-2147483648\n'),
            TestCase('casting.FloatToLong',
                     stdout='42\n-42\n0\n9223372036854775807\n0\n-9223372036854775808\n9223372036854775807\n-9223372036854775808\n0\n9223372036854775807\n-9223372036854775808\n'),
            TestCase('casting.FloatToDouble',
                     stdout='42.9\n-42.9\n0\n1.4013e-45\n3.40282e+38\n-3.40282e+38\ninf\n-inf\nnan\n0.1\n'),
            TestCase('casting.DoubleToInt',
                     stdout='42\n-42\n0\n2147483647\n0\n-2147483648\n2147483647\n-2147483648\n0\n2147483647\n-2147483648\n'),
            TestCase('casting.DoubleToLong',
                     stdout='42\n-42\n0\n9223372036854775807\n0\n-9223372036854775808\n9223372036854775807\n-9223372036854775808\n0\n9223372036854775807\n-9223372036854775808\n'),
            TestCase('casting.DoubleToFloat',
                     stdout='42.9\n-42.9\n0\n0\ninf\n-inf\ninf\n-inf\nnan\n0.1\n'),
        ])

    def run(self):
        self.failures = []
        self.simple_math_programs()
        self.strings()
        self.arrays()
        self.oop_programs()
        self.init()
        self.casting()
        self.exceptions()
        self.errors()
        self.reflection()

    def execute_tests(self, test_suite_name, tests: List[TestCase]):
        print(test_suite_name)
        for test_case in tests:
            success: bool = True

            class_name = test_case.name
            expected_stdout = test_case.expected_stdout
            expected_stderr = test_case.expected_stderr

            try:
                r = self.run_geevm_java(f'org.geevm.tests.{class_name}')
                if r.returncode != 0:
                    print(f'  [{Color.RED}ERROR{Color.RESET}] {class_name}')
                    print(
                        f'  {Color.RED}The geevm java command failed with status code {r.returncode}{Color.RESET}:')
                    print(r.stderr.decode())
                    success = False
                else:
                    if expected_stdout is not None:
                        actual = r.stdout.decode()
                        success = self.compare(class_name, 'stdout', expected_stdout, actual)
                    if expected_stderr is not None:
                        actual = r.stderr.decode()
                        success = self.compare(class_name, 'stderr', expected_stderr, actual)
            except TimeoutError:
                print(f'  [{Color.RED}TIMEOUT{Color.RESET}] {class_name}')
                print(
                    f'  {Color.RED}The geevm java failed due to timeout {Color.RESET}:')
                success = False

            if not success:
                self.failures.append(test_case)

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
        if len(test.failures) != 0:
            print(f'Failed {len(test.failures)} tests:')
            for test_case in test.failures:
                print(f'  {test_case.name}')
            exit(1)
