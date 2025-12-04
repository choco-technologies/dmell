# CMake generated Testfile for 
# Source directory: /home/runner/work/dmell/dmell/tests
# Build directory: /home/runner/work/dmell/dmell/build-tests/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(tests_dmell_vars "/home/runner/work/dmell/dmell/build-tests/tests/tests_dmell_vars")
set_tests_properties(tests_dmell_vars PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/dmell/dmell/tests/CMakeLists.txt;62;add_test;/home/runner/work/dmell/dmell/tests/CMakeLists.txt;0;")
add_test(tests_dmell_cmd "/home/runner/work/dmell/dmell/build-tests/tests/tests_dmell_cmd")
set_tests_properties(tests_dmell_cmd PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/dmell/dmell/tests/CMakeLists.txt;62;add_test;/home/runner/work/dmell/dmell/tests/CMakeLists.txt;0;")
add_test(tests_dmell_line "/home/runner/work/dmell/dmell/build-tests/tests/tests_dmell_line")
set_tests_properties(tests_dmell_line PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/dmell/dmell/tests/CMakeLists.txt;62;add_test;/home/runner/work/dmell/dmell/tests/CMakeLists.txt;0;")
subdirs("../_deps/googletest-build")
