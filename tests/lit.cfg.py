import os.path

import lit.formats

config.name = 'geevm'
config.test_format = lit.formats.ShTest()
# config.test_source_root = os.path.join(os.path.dirname(__file__), 'programs')
config.test_source_root = os.path.join(os.path.dirname(__file__), 'programs', 'org', 'geevm', 'tests')
config.test_exec_root = os.path.join(config.binary_dir, 'tests')
config.suffixes = ['.java', '.j']

config.excludes = ['CMakeLists.txt', '*.class']

config.environment["JAVA_HOME"] = config.java_home
config.environment["GEEVM_BINARY_DIR"] = config.binary_dir
config.environment["GEEVM_TEST_BASE_DIR"] = os.path.join(os.path.dirname(__file__), 'programs')
config.environment["JASMIN_JAR"] = config.jasmin_jar

config.substitutions.append(('%java', os.path.abspath(os.path.join('../cmake-build-debug', 'java'))))
config.substitutions.append(('%compile', os.path.join(os.path.dirname(__file__), 'compile.py')))
