add_custom_target(
  check
  DEPENDS java
  COMMAND ../run_tests.py --geevm-binary-dir=${CMAKE_BINARY_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
