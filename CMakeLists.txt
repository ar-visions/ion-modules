# ion:modules CMake bootstrap
# use this as a drop-in CMakeLists.txt for your projects
cmake_minimum_required(VERSION 3.3)
include(../ion-modules/main.cmake)
read_package()
project(${package_name})
main()
