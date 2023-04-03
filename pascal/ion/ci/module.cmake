cmake_minimum_required(VERSION 3.5)

if (DEFINED ModuleGuard)
    return()
endif()
set(ModuleGuard yes)

if(NOT APPLE)
    find_package(CUDAToolkit QUIET)
endif()

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
    target_include_directories(${t} PUBLIC ${r_path}/${mod})
    if(EXISTS "${r_path}/${mod}/include")
        target_include_directories(${t} PUBLIC ${r_path}/${mod}/include)
    endif()
    target_include_directories(${t} PUBLIC ${r_path})
    target_include_directories(${t} PUBLIC ${CMAKE_BINARY_DIR})
    target_include_directories(${t} PUBLIC "${PREFIX}/include")
endmacro()

macro(set_compilation t mod)
    target_compile_options    (${t} PRIVATE ${CMAKE_CXX_FLAGS} -Wfatal-errors ${cflags})
    target_link_options       (${t} PRIVATE ${lflags})
    target_compile_definitions(${t} PRIVATE ARCH_${UARCH})
    target_compile_definitions(${t} PRIVATE  UNICODE)
    target_compile_definitions(${t} PRIVATE _UNICODE)
    foreach(role ${r_list})
        string(TOUPPER ${role} s)
        target_compile_definitions(${t} PRIVATE ROLE_${s})
    endforeach()
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
    set(roles           "")
    set(dep             "")
    set(src             "")
    set(_includes       "")
    set(_src            "")
    set(_headers        "")
    set(_apps_src       "")
    set(_tests_src      "")
    set(cpp             23)
    set(p "${module_path}")

    # compile these as c++-module (.ixx extension) only on not WIN32
    if(NOT WIN32)
        file(GLOB _src_0 "${p}/*.ixx")
        foreach(isrc ${_src_0})
            set_source_files_properties(${isrc} PROPERTIES LANGUAGE CXX COMPILE_FLAGS "-x c++-module")
        endforeach()
    endif()

    # build the index.ixx first, naturally.  it doesnt need to be some odd source crawling thing but almost certainly needs to be in future (silver)
    if(EXISTS "${p}/${mod}.ixx")
        file(GLOB        _src "${p}/*.ixx" "${p}/*.c*")
        list(REMOVE_ITEM _src "${p}/${mod}.ixx")
        list(INSERT      _src 0 "${p}/${mod}.ixx")
    else()
        file(GLOB        _src "${p}/*.ixx" "${p}/*.c*")
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

    # compile for 23 by default, not experimenting with intra-module differing languages.  its possible, though.
    # a rule can compile 17 if there is a stub .cpp without a .ixx; too restricting it seems, though.
    set(cpp 23)
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
    # 
    foreach(role ${r_list})
        list(FIND roles ${role} role_found)
        if(NOT roles OR (role_found GREATER -1))
            break()
        endif()
    endforeach()

    # if roles listed, and selected build role not listed
    # ------------------------
    if(roles AND role_found EQUAL -1)
        set(br TRUE)
        return()
    endif()
    
    # ------------------------
    list(APPEND m_list ${mod})
    list(FIND arch ${ARCH} arch_found)
    if(arch AND arch_found EQUAL -1)
        set(br TRUE)
        return() # return falsey state
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

# process single dependency
# ------------------------
macro(process_dep d)
    # find 'dot', this indicates a module is referenced (peer modules should be referenced by project)
    # ------------------------
    string(FIND ${d} "." index)

    # project.module supported when the project is imported by peer extension or git relationship
    # ------------------------
    if(index GREATER 0)
        # must exist in extern:
        string(SUBSTRING  ${d} 0 ${index} project)
        math(EXPR index  "${index}+1")
        string(SUBSTRING  ${d} ${index} -1 module)
        set(extern_path   ${CMAKE_BINARY_DIR}/extern/${project})
        if(NOT EXISTS ${extern_path})
            if(${project_name} STREQUAL ${project})
                set(extern_path "${CMAKE_SOURCE_DIR}/${module}")
            endif()
        endif()
        set(pkg_path     "${extern_path}/project.json")
        set(libs         "")
        set(includes     "")
        
        ## if module source
        if(EXISTS ${pkg_path})
            # from peer relationship (symlinked and considered an extension of the active version) 
            set(target           ${project}-${module})
            list(APPEND includes ${extern_path}) # no libs required
            add_dependencies(${t_name} ${target})
        else()
            # imported, .diff applied, generated, built (CMAKE_PATH_PREFIX appends and carries on but i think it needs to be in reverse lookup?)
            set(target ${project}-${module})
            foreach (import ${imports})
                if ("${import}" STREQUAL "${project}")
                    set(libs ${import.${import}.libs})
                    set(includes ${import.${import}.includes})
                endif()
            endforeach()
        endif()

        if(libs)
            link_directories(${t_name} ${libs})
            #message(STATUS "libs = ${libs}")
        endif()
        if(includes)
            target_include_directories(${t_name} PUBLIC ${includes})
            #message(STATUS "includes = ${includes}")
        endif()
        
    else()
        if(NOT WIN32)
            pkg_check_modules(PACKAGE QUIET ${d})
        endif()
        # ---------------------------
        if(PACKAGE_FOUND)
            message(STATUS "pkg_check_modules QUIET -> (target): ${t_name} -> ${d}")
            target_link_libraries(${t_name}             ${PACKAGE_LINK_LIBRARIES})
            target_include_directories(${t_name} PUBLIC ${PACKAGE_INCLUDE_DIRS})
            target_compile_options(${t_name}     PUBLIC ${PACKAGE_CFLAGS_OTHER})
        else()
            # find a cmake package in its many possible forms
            # ---------------------------
            find_package(${d} CONFIG QUIET)
            if(${d}_FOUND)
                if(TARGET ${d}::${d})
                    message(STATUS "find_package CONFIG -> (target): ${t_name} -> ${d}::${d}")
                    target_link_libraries(${t_name} ${d}::${d})
                elseif(TARGET ${d})
                    message(STATUS "find_package CONFIG -> (target): ${t_name} -> ${d}")
                    target_link_libraries(${t_name} ${d})
                endif()
                #
            elseif(TARGET ${d})
                message(STATUS "find_package: (target): ${t_name} -> ${d}")
                target_link_libraries(${t_name} ${d})
            else()
                find_package(${d} QUIET)
                if(${d}_FOUND)
                    message(STATUS "find_package: (target): ${t_name} -> ${d}")
                    if(TARGET ${d}::${d})
                        target_link_libraries(${t_name} ${d}::${d})
                    elseif(TARGET ${d})
                        target_link_libraries(${t_name} ${d})
                    endif()
                elseif(TARGET ${d})
                    message(STATUS "target_link_libraries (target): ${t_name} -> ${d}")
                    target_link_libraries(${t_name} ${d})
                else()
                    message(STATUS "target_link_libraries: (system) ${t_name} -> ${d}")
                    target_link_libraries(${t_name} ${d})
                endif()
                #
            endif()

        endif()
    endif()

    
endmacro()

macro(create_module_targets)

    # generate version stamp for project-module
    # ------------------------
    set(v_src ${CMAKE_BINARY_DIR}/${t_name}-version.cpp)
    set_version_source(${t_name} ${version})
    
    # if there is atleast one source module
    # ------------------------
    if(full_src)
        if (dynamic)
            # if forcing dynamic:
            message(STATUS "a1")
            add_library(${t_name} SHARED ${full_src} ${h_list})
        elseif(static OR external_repo)
            message(STATUS "a2")
            add_library(${t_name} STATIC ${full_src} ${h_list})
        else()
            message(STATUS "a3")
            add_library(${t_name} SHARED ${full_src} ${h_list})
        endif()
        
        if (WIN32)
            # msvc thinks things are too big when you have a lot of inline
            add_compile_options(/bigobj)
        endif()

        if (cpp EQUAL 23)
            if(MSVC)
                target_compile_options(${t_name} PUBLIC /experimental:module /sdl- /EHsc)
            endif()
            target_compile_features(${t_name} PUBLIC cxx_std_23)
        elseif(cpp EQUAL 14)
            target_compile_features(${t_name} PUBLIC cxx_std_14)
        elseif(cpp EQUAL 17)
            if(MSVC)
                target_compile_options (${t_name} PUBLIC /sdl- /EHsc)
            endif()
            target_compile_features(${t_name} PUBLIC cxx_std_17)
        elseif(cpp EQUAL 20)
            target_compile_features(${t_name} PUBLIC cxx_std_20)
        else()
            message(FATAL_ERROR "cpp version ${cpp} not supported")
        endif()
        
        if(full_includes)
            target_include_directories(${t_name} PUBLIC ${full_includes})
        endif()
    else()
        # create stub module (has version function) so we can depend on our own things in the case of nvidia:cuda and such
        # ------------------------
        list(APPEND full_src ${v_src})
        add_library(${t_name} STATIC ${full_src} ${h_list})
    endif()

    set_target_properties(${t_name} PROPERTIES LINKER_LANGUAGE CXX)

    # app products
    # ------------------------
    set_compilation(${t_name} ${mod})
    foreach(app ${apps})
        set(app_path "${p}/apps/${app}")
        string(REGEX REPLACE "\\.[^.]*$" "" t_app ${app})
        list(APPEND app_list ${t_app})
        
        # add app target
        # ------------------------
        #add_executable(${t_app} ${app_path} ${${t_app}_src})
        # allow for adaptation here
        if(cuda_add_executable)
            cuda_add_executable(${t_app} ${app_path} ${${t_app}_src})
        else()
            add_executable(${t_app} ${app_path} ${${t_app}_src}) # must be a setting per app, so store in map or something win32(+app)
        endif()
        # add pre-compiled headers
        # ------------------------
        #target_precompile_headers(${t_app} PUBLIC ${_headers})
        
        # add include dir of apps
        # ------------------------
        target_include_directories(${t_app} PUBLIC ${p}/apps)

        # setup module includes
        # ------------------------
        module_includes(${t_app} ${r_path} ${mod})

        # common compilation flags
        # ------------------------
        set_compilation(${t_app} ${mod})

        # link app to its own module
        # ------------------------
        target_link_libraries(${t_app} ${t_name})

        # format upper-case name for app name
        # ------------------------
        string(TOUPPER ${t_app} u)
        string(REPLACE "-" "_" u ${u})

        # define global name-here -> APP_NAME_HERE
        # ------------------------
        target_compile_definitions(${t_app} PRIVATE APP_${u})
        add_dependencies(${t_app} ${t_name})
        foreach(role ${r_list})
            add_dependencies(${role} ${t_app})
        endforeach()

    endforeach()

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
        target_include_directories(${test_target} PUBLIC ${p}/apps)
        module_includes(${test_target} ${r_path} ${mod})
        set_compilation(${test_target} ${mod})
        target_link_libraries(${test_target} ${t_name})
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

        # add roles in module to these targets (likely singular is preferred)
        # ------------------------
        foreach(role ${r_list})
            add_dependencies(${test_target} ${role})
        endforeach()

    endforeach()

    module_includes(${t_name} ${r_path} ${mod})
    #set_property(TARGET ${t_name} PROPERTY POSITION_INDEPENDENT_CODE ON) # not sure if we need it on with static here
    
    # do not build unless built upon, for the external fellows..
    # if all is built it only wont be built if dynamically unselected from another module
    # ------------------------
    if(external_repo)
        set_target_properties(${t_name} PROPERTIES EXCLUDE_FROM_ALL TRUE)
    endif()

    # iterate through dependencies
    # ------------------------
    if(dep)
        foreach(d ${dep})
            process_dep(${d})
        endforeach()
    endif()
endmacro()

# load module file for a given project (mod, placed in module-folders)
# ------------------------
function(load_module r_path project_name mod)

    # create a target name for this module (t_name = project-module)
    set(t_name "${project_name}-${mod}")
    
    # all modules make targets, so thats our lazy load indicator
    # ------------------------
    if(NOT TARGET ${t_name})
        # ------------------------
        var_prepare(${r_path})
        include(${module_file})
        var_finish()

        # the mod file may need to run twice; that is, if it relies on vars coming from a CMake package it references in dep()
        # ------------------------
        set(retrial FALSE)

        # package management-based priming read
        # proceed as if we would process this in module target creation, for the d's of CMake package
        # ------------------------
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
        # ------------------------
        if(retrial)
            var_prepare(${r_path})
            include(${module_file})
            var_finish()
        endif()

        # create targets now that we have our vars properly set..
        # ------------------------
        create_module_targets()

    endif()

endfunction()


