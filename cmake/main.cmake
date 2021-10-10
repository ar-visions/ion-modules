cmake_minimum_required(VERSION 3.3)
include(../ion-modules/cmake/json.cmake)
include(../ion-modules/cmake/defs.cmake)
include(../ion-modules/cmake/module.cmake)
include(../ion-modules/cmake/package.cmake)

set(mread "" CACHE INTERNAL FORCE DESCRIPTION "")

function(main)
    set_defs()
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/extern)
    load_modules(${CMAKE_CURRENT_SOURCE_DIR} "")
endfunction()
