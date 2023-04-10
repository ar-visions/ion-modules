cmake_minimum_required(VERSION 3.26)

include(../ion/ci/json.cmake)
include(../ion/ci/defs.cmake)
include(../ion/ci/module.cmake)
include(../ion/ci/package.cmake)
include(../ion/ci/cxx20.cmake)

function(main)
    set_defs()
    ## extend cmake to do a bit more importing and building if it has not been done yet
    ## do it in the configuration made by the user and in the output directory of the user
    set(ENV{CMAKE_SOURCE_DIR}   ${CMAKE_SOURCE_DIR})
    set(ENV{CMAKE_BINARY_DIR}   ${CMAKE_BINARY_DIR})
    set(ENV{CMAKE_BUILD_TYPE}   ${CMAKE_BUILD_TYPE})
    set(ENV{JSON_IMPORT_INDEX} "${CMAKE_BINARY_DIR}/import.json")

    # requires ion framework at peer directory level, to prepare
    execute_process(
        COMMAND python3 "${CMAKE_SOURCE_DIR}/../ion/ci/prepare.py"
        RESULT_VARIABLE import_result)

    if (NOT (import_result EQUAL "0"))
        message(SEND_ERROR "could not import dependencies for project ${r_path} (code: ${import_result})")
    endif()

    file(READ $ENV{JSON_IMPORT_INDEX} import_contents)
    sbeParseJson(import_index import_contents)

    set(imports "" CACHE INTERNAL "")

    set(i 0)
    while(true)
        set(s_entry "${import_index_${i}}"      CACHE INTERNAL "")
        set(n_entry "${import_index_${i}.name}" CACHE INTERNAL "")

        if (s_entry)
            list(APPEND imports ${s_entry})
            set(import.${s_entry}          ${s_entry} CACHE INTERNAL "")
            set(import.${s_entry}.extern   ${CMAKE_BINARY_DIR}/extern/${s_entry} CACHE INTERNAL "")
            set(import.${s_entry}.peer     true  CACHE INTERNAL "")
            set(import.${s_entry}.version  "dev" CACHE INTERNAL "")
            
        elseif(n_entry)
            list(APPEND imports ${n_entry})
            set(import.${n_entry}          ${n_entry} CACHE INTERNAL "")
            
            set(import.${n_entry}.peer     false CACHE INTERNAL "")
            set(import.${n_entry}.version  ${import_index_${i}.version} CACHE INTERNAL "")
            set(import.${n_entry}.url      ${import_index_${i}.url} CACHE INTERNAL "")
            set(import.${n_entry}.commit   ${import_index_${i}.commit} CACHE INTERNAL "")
            set(import.${n_entry}.includes "" CACHE INTERNAL "")
            set(import.${n_entry}.libs     "" CACHE INTERNAL "")

            set(import.${n_entry}.extern   ${CMAKE_BINARY_DIR}/extern/${n_entry}-${import_index_${i}.version} CACHE INTERNAL "")

            # add include paths
            set(ii 0)
            while(true)
                #message(STATUS "checking import.${n_entry}.includes += ${import_index_${i}.includes_${ii}}")
                set(include ${import_index_${i}.includes_${ii}} CACHE INTERNAL "")
                if(NOT include)
                    break()
                endif()
                list(APPEND import.${n_entry}.includes ${import.${n_entry}.extern}/${include})
                math(EXPR ii "${ii} + 1")
            endwhile()

            # add lib paths
            set(ii 0)
            while(true)
                set(lib ${import_index_${i}.libs_${ii}})
                if(NOT lib)
                    break()
                endif()
                list(APPEND import.${n_entry}.libs ${import.${n_entry}.extern}/${lib})
                math(EXPR ii "${ii} + 1")
            endwhile()
        else()
            break()
        endif()
        math(EXPR i "${i} + 1")
    endwhile()


    # proj:module -> build-release/extern/proj/module
    foreach(import ${imports})
        if(import.${import}.peer)
            load_project(${import.${import}.extern} "")
        endif()
    endforeach()

    # load primary
    load_project(${CMAKE_CURRENT_SOURCE_DIR} "")
endfunction()
