# ion:modules CMake bootstrap
# use this as a drop-in CMakeLists.txt for your projects
cmake_minimum_required(VERSION 3.3)
include(../ion-modules/cmake/main.cmake)
read_package("package.json" name u v s)
project(${name})
main()
target_precompile_headers(ux-app PRIVATE
  "$<$<COMPILE_LANGUAGE:C>:<stddef.h$<ANGLE-R>>"
  "$<$<COMPILE_LANGUAGE:CXX>:<cstddef$<ANGLE-R>>"
)