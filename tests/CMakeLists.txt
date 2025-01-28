FetchContent_Declare(jasmin
  GIT_REPOSITORY "https://github.com/davidar/jasmin"
  GIT_TAG 2.4
)

FetchContent_MakeAvailable(jasmin)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
  @ONLY
)

add_test(NAME check COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run_lit.py "${CMAKE_CURRENT_BINARY_DIR}" -v)
