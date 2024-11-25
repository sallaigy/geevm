#!/usr/bin/env python3
import argparse
import os
import pathlib
import shutil
import subprocess
import sys

GEEVM_TEST_NAMESPACE = 'org/geevm'


def execute_javac(files: list[str]):
    javac = f'{os.environ["JAVA_HOME"]}/bin/javac'
    javac_command = [javac, '-source', '8', '-target', '8']
    for file in files:
        javac_command.append(file)

    result = subprocess.run(javac_command, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    if result.returncode != 0:
        raise RuntimeError("javac failed: " + result.stderr.decode())


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('inputs', nargs='+')
    parser.add_argument('-d', '--directory', required=True, type=str)
    parser.add_argument('-m', '--main', type=str)
    parser.add_argument('--no-copy-sources', action='store_true', default=False)
    parser.add_argument('-v', '--verbose', action='store_true')

    base_dir = os.environ['GEEVM_TEST_BASE_DIR']
    if base_dir is None:
        print('GEEVM_TEST_BASE_DIR environment variable must be set', file=sys.stderr)
        sys.exit(1)

    base_path = pathlib.Path(base_dir)
    utils_path = base_path / 'org' / 'geevm' / 'util'

    args = parser.parse_args()

    verbose = args.verbose
    destdir = pathlib.Path(args.directory)

    # Copy all needed classes to the destination directory
    target_utils_dir = destdir / 'org' / 'geevm' / 'util'
    os.makedirs(target_utils_dir, exist_ok=True)
    for file in pathlib.Path(utils_path).absolute().glob('./**/*.java'):
        shutil.copy(file, target_utils_dir)

    if not args.no_copy_sources:
        for file in args.inputs:
            resolved_path = pathlib.Path(file).relative_to(base_path)
            resolved_path_dir = resolved_path.parent

            os.makedirs(destdir / resolved_path_dir, exist_ok=True)
            shutil.copy(file, destdir / resolved_path_dir)

    java_files = [str(path) for path in destdir.glob('./**/*.java')]
    execute_javac(java_files)

    # Execute the geevm binary
    java_tool_path = f'{os.environ['GEEVM_BINARY_DIR']}/java'

    if args.main is not None:
        main_class = args.main
    elif len(args.inputs) == 1:
        resolved_path = pathlib.Path(args.inputs[0]).relative_to(base_path).with_suffix('')
        main_class = str(resolved_path).replace(os.pathsep, '.')
    else:
        raise RuntimeError("main must be set!")

    java_command = [java_tool_path, main_class]
    if verbose:
        print(f'Running java command: {java_command}')
    r = subprocess.run(java_command, stdout=sys.stdout, stderr=sys.stderr, cwd=destdir)
    if verbose:
        print(f'Returned: {r}')

    if r.returncode != 0:
        exit(r.returncode)
