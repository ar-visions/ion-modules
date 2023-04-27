cmake_minimum_required(VERSION 3.26)

include(../ion/ci/json.cmake)
include(../ion/ci/defs.cmake)
include(../ion/ci/module.cmake)
include(../ion/ci/package.cmake)
## clang++ 
## include(../ion/ci/cxx20.cmake)

function(unzip ZIP_FILE_PATH OUTPUT_DIR)
    if (UNIX)
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xzf ${ZIP_FILE_PATH}
            WORKING_DIRECTORY ${OUTPUT_DIR}
            RESULT_VARIABLE unzip_result)
    elseif (WIN32)
        execute_process(
            COMMAND powershell -nologo -noprofile -command "& {Add-Type -A 'System.IO.Compression.FileSystem'; [IO.Compression.ZipFile]::ExtractToDirectory('${ZIP_FILE_PATH}', '${OUTPUT_DIR}');}"
            RESULT_VARIABLE unzip_result)
    endif()
    if (NOT unzip_result EQUAL 0)
        message(FATAL_ERROR "Failed to unzip the file.")
    endif()
endfunction()

macro(assert COND DESC)
    if(NOT ${COND})
        message(FATAL_ERROR "assertion failure: " ${DESC})
    endif()
endmacro()

macro(assert_exists FILE DESC)
    if(NOT EXISTS ${FILE})
        message(FATAL_ERROR "file not found: " ${DESC})
    endif()
endmacro()

# bootstrap python.  it takes 3 seconds to install the proper python for any system and avoid dx bloat
# environment is the build dir. its all we need
# they need to start distributing binaries in tar.gz form without the installer and registry cruft.  doing this manually because
# invoking python installer even in silent with targeted dir will corrupt the registry
# this method is suitable until proper binaries can be packaged
# one could build python from source but that would be a bit much
macro(bootstrap_python)
    set_if(EXT WIN32 ".EXE" "")
    if(WIN32)
        set(SLA "\\")
    else()
        set(SLA "/")
    endif()
    set_if(SEP WIN32 "\;" ":")

    set(PYTHON_VERSION "3.9.13")
    set(RES_DIR        ${CMAKE_CURRENT_BINARY_DIR}/io/res)
    set(PYTHON_PACKAGE ${RES_DIR}/python-${PYTHON_VERSION}-${OS}.zip)
    set(PYTHON_PATH    ${CMAKE_BINARY_DIR}/python-${PYTHON_VERSION})
    set(PYTHON         ${PYTHON_PATH}/python${EXT})
    set(PIP            ${PYTHON_PATH}/Scripts/pip${EXT})

    set(ENV{PATH} "${PYTHON_PATH}\;${PYTHON_PATH}\\Scripts\;$ENV{PATH}")

    if(NOT EXISTS ${PYTHON})
        if(NOT EXISTS io)
            execute_process(COMMAND git clone "https://github.com/ar-visions/ar-visions.github.io" io)
        endif()
        unzip(${PYTHON_PACKAGE} ${CMAKE_BINARY_DIR})
        assert_exists(${PIP} "pip")
        foreach(pkg ${ARGV})
            message(STATUS "installing pip package: ${pkg} (${PIP})")
            message(STATUS "ENV: " $ENV{PATH})
            execute_process(COMMAND ${PIP} install ${pkg} RESULT_VARIABLE pkg_res)
            message(STATUS "result: ${pkg_res}")
        endforeach()
        assert_exists(${PYTHON} "python not found")
    endif()

endmacro()

function(main)
    set_defs()

    ## extend cmake to do a bit more importing and building if it has not been done yet
    ## do it in the configuration made by the user and in the output directory of the user
    set(ENV{CMAKE_SOURCE_DIR}   ${CMAKE_SOURCE_DIR})
    set(ENV{CMAKE_BINARY_DIR}   ${CMAKE_BINARY_DIR})
    set(ENV{CMAKE_BUILD_TYPE}   ${CMAKE_BUILD_TYPE})
    set(ENV{JSON_IMPORT_INDEX} "${CMAKE_BINARY_DIR}/import.json")

    # bootstrap python with pip packages
    bootstrap_python(requests)

    # now that we have prepared python.. we can run prepare.py
    execute_process(
        COMMAND ${PYTHON} "${CMAKE_SOURCE_DIR}/../ion/ci/prepare.py"
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

                # msvc has one additional dir which is switched on standard CMAKE_BUILD_TYPE
                # env var passed to prepare.py, passed onto external cmake projects
                set(extra "")
                if(MSVC)
                    set(extra "/Release")
                    if(DEBUG)
                        set(extra "/Debug")
                    endif()
                endif()
                list(APPEND import.${n_entry}.libs ${import.${n_entry}.extern}/${lib}${extra})
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

    # function to print library paths for a given target
    #function(print_library_paths target_name)
    #    get_target_property(link_libraries ${target_name} LINK_LIBRARIES)
    #    message(STATUS "Library paths for target ${target_name}:")
    #
    #    foreach(lib ${link_libraries})
    #        if(TARGET ${lib})
    #        get_target_property(lib_type ${lib} TYPE)
    #        if(${lib_type} STREQUAL "INTERFACE_LIBRARY")
    #            get_target_property(lib_dirs ${lib} INTERFACE_LINK_DIRECTORIES)
    #        else()
    #            get_target_property(lib_dirs ${lib} LINK_DIRECTORIES)
    #        endif()
    #        if(lib_dirs)
    #            foreach(dir ${lib_dirs})
    #            message(STATUS "  -L ${dir}")
    #            endforeach()
    #        endif()
    #        endif()
    #    endforeach()
    #    message(STATUS "")
    #endfunction()
    #get_property(all_targets DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY BUILDSYSTEM_TARGETS)
    #foreach(target ${all_targets})
    #    print_library_paths(${target})
    #endforeach()

endfunction()
