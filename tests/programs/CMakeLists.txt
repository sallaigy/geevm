add_custom_target(check DEPENDS java)

add_custom_command(
  TARGET check
  COMMAND ../run_tests.py
  ARGS --geevm-binary-dir=${CMAKE_BINARY_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
