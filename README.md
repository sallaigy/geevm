# GeeVM

GeeVM is a JDK 17-compatible JVM implementation in C++, implemented as a learning project.
It comes with full OpenJDK compatibility, JNI support and a copying garbage collector.

## Building

### Requirements

You'll require the following dependencies:

- C++23-compatible compiler,
- CMake 3.28 or newer,
- OpenJDK/OracleJDK 17,
- `libzip`.

GeeVM currently only supports on Linux.

### Building with CMake

The CMake build requires OpenJDK/OracleJDK 17 to be present on the system in
order to extract and copy the required classes from the JDK. By default, it will
search in the directory specified by the environment variable `JAVA_HOME`.

After installing the dependencies and setting `JAVA_HOME` accordingly, you can build with the usual CMake commands:

```shell
cmake -B build -S . 
cmake --build build
```

### Running the tests

Test files are full Java programs written either in Java or [Jasmin](https://github.com/davidar/jasmin)
and are found in the `tests/programs` directory.

Test execution is done using LLVM's [lit](https://llvm.org/docs/CommandGuide/lit.html)
and [FileCheck](https://llvm.org/docs/CommandGuide/FileCheck.html) tools that need to be installed separately.
Once they are installed, you can run the full test suite using `ctest`:

```shell
cmake --build build --target test
```

## Usage

Run the built `java` executable:

```
java <mainclass> [args]...
```

## References

- [JVM 17 Specification](https://docs.oracle.com/javase/specs/jvms/se17/html/index.html)
- [JNI 17 Specification](https://docs.oracle.com/en/java/javase/17/docs/specs/jni/)
