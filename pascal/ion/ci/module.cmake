cmake_minimum_required(VERSION 3.26)

if (DEFINED ModuleGuard)
    return()
endif()
set(ModuleGuard yes)

if(NOT APPLE)
    find_package(CUDAToolkit QUIET)
endif()

macro(cpp ver)
    set(cpp ${ver})
endmacro()

set_property(GLOBAL PROPERTY GLOBAL_DEPENDS_DEBUG_MODE 1)

# generate some C source for version
function(set_version_source p_name p_version)
    set(v_src ${CMAKE_BINARY_DIR}/${p_name}-version.cpp)
    if(NOT ${p_name}-version-output)
        set_if(fn_attrs WIN32 "" "__attribute__((visibility(\"default\")))")
        string(TOUPPER ${p_name} u)
        string(REPLACE "-" "_"   u ${u})
        set(c_src "extern ${fn_attrs} const char *${u}_VERSION() { return \"${p_version}\"\; }")
        file(WRITE ${v_src} ${c_src})
        set(${p_name}-version-output TRUE)
    endif()
endfunction()

# standard module includes
macro(module_includes t r_path mod)
    #target_include_directories(${t} PRIVATE ${r_path}/${mod})
    #if(EXISTS "${r_path}/${mod}/include")
    #    target_include_directories(${t} PRIVATE ${r_path}/${mod}/include)
    #endif()
    target_include_directories(${t} PRIVATE ${r_path})
    target_include_directories(${t} PRIVATE ${CMAKE_BINARY_DIR})
    target_include_directories(${t} PRIVATE "${PREFIX}/include")
endmacro()

macro(set_compilation t mod)
    target_compile_options(${t} PRIVATE ${CMAKE_CXX_FLAGS} ${cflags})
    if(Clang)
        target_compile_options(${t} PRIVATE -Wfatal-errors)
    endif()
    target_link_options       (${t} PRIVATE ${lflags})
    target_compile_definitions(${t} PRIVATE ARCH_${UARCH})
    target_compile_definitions(${t} PRIVATE  UNICODE)
    target_compile_definitions(${t} PRIVATE _UNICODE)
endmacro()

macro(app_source app)
    if (NOT external_repo)
        listy(${app}_source "${mod}/apps/" ${ARGN})
    else()
        listy(${app}_source "${r_path}/${mod}/apps/" ${ARGN})
    endif()
endmacro()

macro(var_prepare r_path)
    set(module_path     "${r_path}/${mod}")
    set(module_file     "${module_path}/mod")
    set(arch            "")
    set(cflags          "")
    set(lflags          "")
    #set(roles "")
    set(dep             "")
    set(src             "")
    set(_includes       "")
    set(_src            "")
    set(_headers        "")
    set(_apps_src       "")
    set(_tests_src      "")
    set(cpp             20)
    set(p "${module_path}")

    # compile these as c++-module (.ixx extension) only on not WIN32
    if(NOT WIN32)
        file(GLOB _src_0 "${p}/*.ixx")
        #foreach(isrc ${_src_0})
        #    set_source_files_properties(${isrc} PROPERTIES LANGUAGE CXX COMPILE_FLAGS "-x c++-module")
        #endforeach()
    endif()

    # build the index.ixx first, naturally.  it doesnt need to be some odd source crawling thing but almost certainly needs to be in future (silver)
    if(EXISTS "${p}/${mod}.ixx" OR EXISTS "${p}/${mod}.cpp")
        file(GLOB        _src "${p}/*.ixx" "${p}/*.c*")
        list(REMOVE_ITEM _src "${p}/${mod}.ixx")
        list(REMOVE_ITEM _src "${p}/${mod}.cpp")
        if(EXISTS "${p}/${mod}.cpp")
            list(INSERT _src 0 "${p}/${mod}.cpp")
        endif()
        if(EXISTS "${p}/${mod}.ixx")
            list(INSERT _src 0 "${p}/${mod}.ixx")
        endif()
    else()
        file(GLOB _src "${p}/*.ixx" "${p}/*.c*")
    endif()

    file(GLOB _headers           "${p}/*.h*")
    if(EXISTS                    "${p}/apps")
        file(GLOB _apps_src      "${p}/apps/*.c*")
        file(GLOB _apps_headers  "${p}/apps/*.h*")
    endif()
    if(EXISTS                    "${p}/tests")
        file(GLOB _tests_src     "${p}/tests/*.c*")
        file(GLOB _tests_headers "${p}/tests/*.h*")
    endif()
    
    # filename component
    set(src "")
    foreach(s ${_src})
        get_filename_component(name ${s} NAME)
        list(APPEND src ${name})
    endforeach()
    
    # list of header-dirs
    set(headers "")
    foreach(s ${_headers})
        get_filename_component(name ${s} NAME)
        list(APPEND headers ${name})
    endforeach()

    # compile for 17 by default
    set(cpp 17)
    set(dynamic false)

    # list of app targets
    set(apps "")
    foreach(s ${_apps_src})
        get_filename_component(name ${s} NAME)
        list(APPEND apps ${name})
    endforeach()
    
    # list of header-dirs for apps (all)
    set(apps_headers "")
    foreach(s ${_apps_headers})
        get_filename_component(name ${s} NAME)
        list(APPEND apps_headers ${name})
    endforeach()
    
    # list of test targets
    set(tests "")
    foreach(s ${_tests_src})
        get_filename_component(name ${s} NAME)
        list(APPEND tests ${name})
    endforeach()
    set(tests_headers "")
    foreach(s ${_tests_headers})
        get_filename_component(name ${s} NAME)
        list(APPEND tests_headers ${name})
    endforeach()

endmacro()

macro(var_finish)

    # ------------------------
    list(APPEND m_list ${mod})
    list(FIND arch ${ARCH} arch_found)
    if(arch AND arch_found EQUAL -1)
        set(br TRUE)
        return()
    endif()

    # ------------------------
    set(full_src "")
    #list(APPEND full_src "${p}/mod") -- just some Xcode hack
    foreach(n ${src})
        list(APPEND full_src "${p}/${n}")
    endforeach()

    # h_list (deprecate)
    # ------------------------
    set(h_list "")
    foreach(n ${headers})
        list(APPEND h_list "${p}/${n}")
    endforeach()
    
    set(full_includes "")

    # add prefixed location if it exists
    # ------------------------
    if(EXISTS "${PREFIX}/include")
        list(APPEND full_includes "${PREFIX}/include")
    endif()

    # handle includes by preferring abs path, then relative to prefix-path
    # ------------------------
    foreach(n ${includes})
        set(found FALSE)

        # abs path n exists
        # ------------------------
        if(EXISTS ${n})
            list(APPEND full_includes ${n})
            set(found TRUE)
        endif()

        # /usr/local/include/n exists (system-include)
        # ------------------------
        if(EXISTS "${PREFIX}/include/${n}")
            list(APPEND full_includes "${PREFIX}/include/${n}")
            set(found TRUE)
        endif()

        # include must be found; otherwise its indication of error
        # ------------------------
        if(!found)
            message(FATAL "couldnt find include path for symbol: ${n}")
        endif()
    endforeach()
endmacro()

function(get_name location name)
    file(READ ${location} package_contents)
    sbeParseJson(package package_contents)
    set(${name} ${package.name} PARENT_SCOPE)
endfunction()

# name name_ucase version imports
function(read_project_json location name name_ucase version)
    ##
    file(READ ${location} package_contents)

    if(NOT package_contents)
        message(FATAL_ERROR "project.json not in package root")
    endif()

    ## parse json with sbe-parser (GPLv3)
    sbeParseJson(package package_contents)

    set(${name}    ${package.name}    PARENT_SCOPE)
    set(${version} ${package.version} PARENT_SCOPE)

    ## format the package-name from this-case to THIS_CASE
    string(TOUPPER       ${package.name}    pkg_name_ucase)
    string(REPLACE "-" "_" pkg_name_ucase ${pkg_name_ucase})

    if(name_ucase)
        set(${name_ucase} ${pkg_name_ucase} PARENT_SCOPE)
    endif()

    sbeClearJson(package)
endfunction()

macro(get_module_dirs src result)
    file(GLOB children RELATIVE ${src} ${src}/*)
    set(dirlist "")
    foreach(child ${children})
    if(IS_DIRECTORY "${src}/${child}" AND EXISTS "${src}/${child}/mod")
        list(APPEND dirlist ${child})
    endif()
    endforeach()
    set(${result} ${dirlist})
endmacro()

function(load_project location remote)
    # return right away if this project is not ion-oriented
    set(js ${location}/project.json)
    if(NOT EXISTS ${js})
        return()
    endif()

    # ------------------------
    read_project_json(${js} name name_ucase version)

    # fetch a list of all module dir stems (no path behind, no ./ or anything just the names)
    # ------------------------
    get_module_dirs(${location} module_dirs)

    # ------------------------
    set(m_list    "" PARENT_SCOPE)
    set(app_list  "" PARENT_SCOPE)
    set(test_list "" PARENT_SCOPE)

    # iterate through modules
    # ------------------------
    foreach(mod ${module_dirs})
        load_module(${location} ${name} ${mod})
    endforeach()
    
endfunction()

# select cmake package, target name, or framework
# can likely deprecate the target name case
function(select_library t_name d)
    find_package(${d} CONFIG QUIET)
    if(${d}_FOUND)
        if(TARGET ${d}::${d})
            message(STATUS "(package) ${t_name} -> ${d}::${d}")
            target_link_libraries(${t_name} PRIVATE ${d}::${d})
        elseif(TARGET ${d})
            message(STATUS "(package) ${t_name} -> ${d}")
            target_link_libraries(${t_name} PRIVATE ${d})
        endif()
        #
    elseif(TARGET ${d})
        message(STATUS "(target) ${t_name} -> ${d}")
        target_link_libraries(${t_name} PRIVATE ${d})
    else()
        find_package(${d} QUIET)
        if(${d}_FOUND)
            message(STATUS "(package) ${t_name} -> ${d}")
            if(TARGET ${d}::${d})
                target_link_libraries(${t_name} PRIVATE ${d}::${d})
            elseif(TARGET ${d})
                target_link_libraries(${t_name} PRIVATE ${d})
            endif()
        else()
            find_library(${d}_FW ${d} QUIET)
            if(${d}_FW)
                message(STATUS "(framework) ${t_name} -> ${${d}_FW}")
                target_link_libraries(${t_name} PRIVATE ${${d}_FW})
            else()
                message(STATUS "(system) ${t_name} -> ${d}")
                target_link_libraries(${t_name} PRIVATE ${d})
            endif()
        endif()
        #
    endif()
endfunction()

## process single dependency
## ------------------------
macro(process_dep d t_name)
    ## find 'dot', this indicates a module is referenced (peer modules should be referenced by project)
    ## ------------------------
    string(FIND ${d} "." index)

    ## project.module supported when the project is imported by peer extension or git relationship
    ## ------------------------
    if(index GREATER 0)
        ## must exist in extern:
        string(SUBSTRING  ${d} 0 ${index} project)
        math(EXPR index   "${index}+1")
        string(SUBSTRING  ${d} ${index} -1 module)
        set(extern_path   ${CMAKE_BINARY_DIR}/extern/${project})
        set(pkg_path      "")
        ##
        if(NOT EXISTS ${extern_path})
            if(${project_name} STREQUAL ${project})
                # must assert that this project is a 'peer' directory in its project.json; set a bool for that
                set(extern_path "${CMAKE_SOURCE_DIR}/../${project}/${module}")
                set(pkg_path    "${CMAKE_SOURCE_DIR}/../${project}/project.json")
            endif()
        endif()

        # if module source
        if(EXISTS ${pkg_path})
            set(target ${project}-${module})
            target_link_libraries(${t_name} PRIVATE ${target})
            target_include_directories(${t_name} PRIVATE ${extern_path})
        else()
            set(found FALSE)
            foreach (import ${imports})
                if ("${import}" STREQUAL "${project}")
                    # add lib paths for this external
                    foreach (i ${import.${import}.libs})
                        link_directories(${t_name} ${i})
                    endforeach()

                    # switch based on static/dynamic use-cases
                    # private for shared libs and public for pass-through to exe linking
                    set_if(exposure dynamic "PRIVATE" "PUBLIC")
                    target_link_libraries(${t_name} ${exposure} ${module}) 
                    target_include_directories(${t_name} PRIVATE ${import.${import}.includes})
                    set(found TRUE)
                    break()
                endif()
            endforeach()
            if (NOT found)
                message(FATAL_ERROR "external module not found")
            endif()
        endif()
    else()
        if(NOT WIN32)
            pkg_check_modules(PACKAGE QUIET ${d})
        endif()
        if(PACKAGE_FOUND)
            message(STATUS "pkg_check_modules QUIET -> (target): ${t_name} -> ${d}")
            target_link_libraries(${t_name}      PRIVATE ${PACKAGE_LINK_LIBRARIES})
            target_include_directories(${t_name} PRIVATE ${PACKAGE_INCLUDE_DIRS})
            target_compile_options(${t_name}     PRIVATE ${PACKAGE_CFLAGS_OTHER})
        else()
            select_library(${t_name} ${d})
        endif()
    endif()
endmacro()

macro(address_sanitizer t_name)
    # enable address sanitizer
    if (DEBUG)
        if (MSVC)
            target_compile_options(${t_name} PRIVATE /fsanitize=address)
        else()
            target_compile_options(${t_name} PRIVATE -fsanitize=address)
            target_link_options(${t_name} PRIVATE -fsanitize=address)
        endif()
    endif()
endmacro()

macro(create_module_targets)
    set(v_src ${CMAKE_BINARY_DIR}/${t_name}-version.cpp)
    set_version_source(${t_name} ${version})
    if(full_src)
        if (dynamic)
            add_library(${t_name} ${h_list})
            if(cpp EQUAL 23)
                target_sources(${t_name}
                    PRIVATE
                        FILE_SET cxx_modules TYPE CXX_MODULES FILES
                        ${full_src})
            else()
                target_sources(${t_name} PRIVATE ${full_src})
            endif()
        elseif(static OR external_repo)
            add_library(${t_name} STATIC)
            message(STATUS "full_src: ${full_src}")
            if(cpp EQUAL 23)
                #target_sources(${t_name}
                #    PRIVATE
                #        FILE_SET cxx_modules TYPE CXX_MODULES
                #        FILES ${full_src} 
                #    INTERFACE
                #        FILE_SET headers TYPE HEADERS
                #        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/${mod}"
                #        FILES ${h_list})
            else()
                target_sources(${t_name} PRIVATE ${full_src})
            endif()

        else()
            message(FATAL_ERROR "!dynamic && !static")
        endif()
        
        if (WIN32)
            add_compile_options(/bigobj)
        endif()

        address_sanitizer(${t_name})
        
        if (cpp EQUAL 23)
            if(MSVC)
                target_compile_options(${t_name} PRIVATE /experimental:module /sdl- /EHsc)
            endif()
            target_compile_features(${t_name} PRIVATE cxx_std_23)
        elseif(cpp EQUAL 14)
            target_compile_features(${t_name} PRIVATE cxx_std_14)
        elseif(cpp EQUAL 17)
            if(MSVC)
                target_compile_options (${t_name} PRIVATE /sdl- /EHsc)
            endif()
            target_compile_features(${t_name} PRIVATE cxx_std_17)
        elseif(cpp EQUAL 20)
            target_compile_features(${t_name} PRIVATE cxx_std_20)
        else()
            message(FATAL_ERROR "cpp version ${cpp} not supported")
        endif()
        
        if(full_includes)
            target_include_directories(${t_name} PRIVATE ${full_includes})
        endif()
    else()
        list(APPEND full_src ${v_src})
        add_library(${t_name} STATIC ${full_src} ${h_list})
    endif()

    set_target_properties(${t_name} PROPERTIES LINKER_LANGUAGE CXX)

    # app products
    # ------------------------
    set_compilation(${t_name} ${mod})

    # test products
    # ------------------------
    foreach(test ${tests})
        # strip the file stem for its target name
        # ------------------------
        get_filename_component(e ${test} EXT)
        string(REGEX REPLACE "\\.[^.]*$" "" test_name ${test})
        set(test_file "${p}/tests/${test}")
        set(test_target ${test_name})
        list(APPEND test_list ${test})
        add_executable(${test_target} ${test_file})
    
        # add apps as additional include path, standard includes and compilation settings for module
        # ------------------------
        target_include_directories(${test_target} PRIVATE ${p}/apps)
        module_includes(${test_target} ${r_path} ${mod})
        set_compilation(${test_target} ${mod})
        target_link_libraries(${test_target} PRIVATE ${t_name})
        # ------------------------
        if(NOT TARGET test-${t_name})
            add_custom_target(test-${t_name})
            if (NOT TARGET test-all)
                add_custom_target(test-all)
            endif()
            add_dependencies(test-all test-${t_name})
        endif()
    
       # add this test to the test-proj-mod target group
       # ------------------------
       add_dependencies(test-${t_name} ${test_target})
    
    endforeach()

    module_includes(${t_name} ${r_path} ${mod})
    #set_property(TARGET ${t_name} PROPERTY POSITION_INDEPENDENT_CODE ON) # not sure if we need it on with static here
    
    if(external_repo)
        set_target_properties(${t_name} PROPERTIES EXCLUDE_FROM_ALL TRUE)
    endif()

    if(dep)
        foreach(d ${dep})
            process_dep(${d} ${t_name})
        endforeach()
    endif()

    foreach(app ${apps})
        set(app_path "${p}/apps/${app}")
        string(REGEX REPLACE "\\.[^.]*$" "" t_app ${app})
        list(APPEND app_list ${t_app})
        
        # add app target
        if(cuda_add_executable)
            cuda_add_executable(${t_app} ${app_path} ${${t_app}_src})
        else()
            add_executable(${t_app} ${app_path} ${${t_app}_src}) # must be a setting per app, so store in map or something win32(+app)
        endif()
        
        address_sanitizer(${t_app})

        # add pre-compiled headers
        # ------------------------
        #target_precompile_headers(${t_app} PRIVATE ${_headers})
        
        # add include dir of apps
        # ------------------------
        target_include_directories(${t_app} PRIVATE ${p}/apps)

        # setup module includes
        # ------------------------
        module_includes(${t_app} ${r_path} ${mod})

        # common compilation flags
        # ------------------------
        set_compilation(${t_app} ${mod})

        # format upper-case name for app name
        # ------------------------
        string(TOUPPER ${t_app} u)
        string(REPLACE "-" "_" u ${u})

        # define global name-here -> APP_NAME_HERE
        # ------------------------
        target_compile_definitions(${t_app} PRIVATE APP_${u})

        target_link_libraries(${t_app} PRIVATE ${t_name}) # public?
        add_dependencies(${t_app} ${t_name})
        
    endforeach()
endmacro()

# load module file for a given project (mod, placed in module-folders)
# ------------------------
function(load_module r_path project_name mod)
    # create a target name for this module (t_name = project-module)
    set(t_name "${project_name}-${mod}")
    
    # all modules make targets, so thats our lazy load indicator
    if(NOT TARGET ${t_name})
        var_prepare(${r_path})
        include(${module_file})
        var_finish()

        # the mod file may need to run twice; that is, if it relies on vars coming from a CMake package it references in dep()
        set(retrial FALSE)

        # package management-based priming read
        # proceed as if we would process this in module target creation, for the d's of CMake package
        foreach(d ${dep})
            find_package(${d} QUIET)
            if (${d}_FOUND)
                set(retrial TRUE)
            else()
                find_package(${d} CONFIG QUIET)
                if (${d}_FOUND)
                    set(retrial TRUE)
                endif()
            endif()
        endforeach()

        # i-see-you-are-using-package-loaders, lets-try-that-again
        if(retrial)
            var_prepare(${r_path})
            include(${module_file})
            var_finish()
        endif()

        # create targets now that we have our vars properly set..
        create_module_targets()
    endif()
endfunction()


