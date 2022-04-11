cmake_minimum_required(VERSION 3.3)

set(CMAKE_CXX_COMPILER "g++-11")

include(../ionm/cmake/json.cmake)
include(../ionm/cmake/defs.cmake)
include(../ionm/cmake/module.cmake)
include(../ionm/cmake/package.cmake)

set(mread "" CACHE INTERNAL FORCE DESCRIPTION "")

function(main)
    set_defs()
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/extern)
    load_modules(${CMAKE_CURRENT_SOURCE_DIR} "")
endfunction()
