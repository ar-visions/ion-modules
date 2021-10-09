cmake_minimum_required(VERSION 3.3)
include(../ion-ci/json.cmake)
include(../ion-ci/defs.cmake)
include(../ion-ci/module.cmake)
include(../ion-ci/package.cmake)

set(mread "" CACHE INTERNAL FORCE DESCRIPTION "")

function(main)
    set_defs()
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/extern)
    load_modules(${CMAKE_CURRENT_SOURCE_DIR} "")
endfunction()
