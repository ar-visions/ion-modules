cmake_minimum_required(VERSION 3.3)

if (DEFINED ModuleGuard)
    return()
endif()
set(ModuleGuard yes)

function(set_version_source p_name)
    if(NOT ${p_name}-version-output)
        set(v_src                             ${CMAKE_BINARY_DIR}/${p_name}-version.cpp)
        set_if(fn_attrs WIN32                 "" "__attribute__((visibility(\"default\")))")
        set(c_src                             "extern ${fn_attrs} const char *${p_name_ucase}_VERSION() { return \"${p_version}\"\; }")
        file(WRITE ${v_src}                   ${c_src})
        add_library(${p_name}-version STATIC  ${v_src})
        set(${p_name}-version-output          TRUE)
    endif()
endfunction()

macro(module_includes t r_path mod)
    target_include_directories(${t} PUBLIC ${r_path}/${mod})
    target_include_directories(${t} PUBLIC ${r_path})
    target_include_directories(${t} PUBLIC ${CMAKE_BINARY_DIR})
    target_include_directories(${t} PUBLIC "${PREFIX}/include")
endmacro()

macro(set_compilation t src mod)
    target_compile_options(${t}     PRIVATE ${cflags})
    target_compile_definitions(${t} PRIVATE ARCH_${UARCH})
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

message(STATUS "asdasdasds")

macro(var_prepare)
    set(module_path     "${r_path}/${mod}")
    set(module_file     "${module_path}/mod")
    set(arch            "")
    
    # setting a cflag to a specific target is important. 
    # its a useless feature otherwise, its a matter of 'where does it apply'
    # and when you want to pass for just a single target in a larger build
    # system, you just dont. that sort of workflow is for command-line.
    # plugging in build attribs in the Orbiter project (for each project loaded into orbiter it would have these:)

    set(cflags          "")
    set(roles           "")
    set(dep             "")
    set(src             "")
    set(_includes       "")
    set(_src            "")
    set(_headers        "")
    set(_apps_src       "")
    set(_tests_src      "")
    if(NOT external_repo)
        set(p "${mod}")
    else()
        set(p "extern/${external_repo}/${mod}")
    endif()

   #file(GLOB _src_cpp           "${p}/*.c*")
    file(GLOB _src_ipp           "${p}/*.ipp")
    file(GLOB _src_ixx           "${p}/*.ixx")
   #set(_src ${_src_cpp} ${_src_ipp} ${_src_ixx})
    set(_src ${_src_ipp} ${_src_ixx})

    file(GLOB _headers           "${p}/*.h*")
    if(EXISTS                    "${CMAKE_CURRENT_SOURCE_DIR}/${p}/apps")
        file(GLOB _apps_src      "${CMAKE_CURRENT_SOURCE_DIR}/${p}/apps/*.c*")
        file(GLOB _apps_headers  "${CMAKE_CURRENT_SOURCE_DIR}/${p}/apps/*.h*")
    endif()
    if(EXISTS                    "${CMAKE_CURRENT_SOURCE_DIR}/${p}/tests")
        file(GLOB _tests_src     "${CMAKE_CURRENT_SOURCE_DIR}/${p}/tests/*.c*")
        file(GLOB _tests_headers "${CMAKE_CURRENT_SOURCE_DIR}/${p}/tests/*.h*")
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
    if(external_repo)
        set(t_name "${external_repo}-${mod}")
    else()
        set(t_name "${mod}")
    endif()

endmacro()

macro(var_finish)
    foreach(role ${r_list})
        list(FIND roles ${role} role_found)
        if(NOT roles OR (role_found GREATER -1))
            break()
        endif()
    endforeach()
    if(roles AND role_found EQUAL -1)
        continue()
    endif()
    list(APPEND m_list ${mod})
    list(FIND arch ${ARCH} arch_found)
    if(arch AND arch_found EQUAL -1)
        continue()
    endif()
    set(full_src "")
    list(APPEND full_src "${p}/mod")
    foreach(n ${src})
        list(APPEND full_src "${p}/${n}")
    endforeach()
    set(h_list "")
    foreach(n ${headers})
        list(APPEND h_list "${p}/${n}")
    endforeach()
    
    set(full_includes "")
    list(APPEND full_includes "${PREFIX}/include")
    foreach(n ${includes})
        list(APPEND full_includes "${PREFIX}/include/${n}")
    endforeach()
endmacro()

function(load_modules r_path external_repo)
    read_package("${r_path}/package.json" pkg_name pkg_name_ucase pkg_version pkg_subdirs)

    subdirs(dirs ${r_path})
    set(mods "")
    foreach(mod ${dirs})
        if(EXISTS "${r_path}/${mod}/mod")
            list(APPEND mods ${mod})
        endif()
    endforeach()
    if (NOT external_repo)
        foreach(role ${r_list})
            add_custom_target(${role})
        endforeach()
    endif()

    set_version_source(${pkg_name})
    set(m_list    "" PARENT_SCOPE)
    set(app_list  "" PARENT_SCOPE)
    set(test_list "" PARENT_SCOPE)

    foreach(mod ${mods})
        var_prepare()
        include(${module_file})
        var_finish()

        foreach(role ${r_list})
            add_dependencies(${role} ${t_name})
        endforeach()

        if(NOT TARGET ${t_name})
            #if(static OR external_repo)
                #add_library(${t_name} STATIC ${full_src} ${h_list})
            #else()
                #add_library(${t_name} SHARED ${full_src} ${h_list})
            #endif()

            # definitely simpler at this level.
            add_library(${t_name} OBJECT ${full_src} ${h_list})
            target_compile_options(${t_name} PUBLIC -fmodules-ts)
            set_property(TARGET ${t_name} PROPERTY CXX_STANDARD 20)
            set_property(TARGET ${t_name} PROPERTY CXX_EXTENSIONS OFF)
            add_dependencies(${t_name} std_modules)
        endif()
        
        if(full_includes)
            target_include_directories(${t_name} PUBLIC ${full_includes})
        endif()
        
        # app products
        set_compilation(${t_name} ${full_src} ${mod})
        foreach(app ${apps})
            set(app_path "${p}/apps/${app}")
            string(REGEX REPLACE "\\.[^.]*$" "" t_app ${app})
            list(APPEND app_list ${t_app})
            
            # add app target
            add_executable(${t_app} ${app_path} ${${t_app}_src})
            add_dependencies(${t_app} std_modules)

            # link
            #target_link_libraries(${t_app} PRIVATE ${t_name})

            # add pre-compiled headers
            #target_precompile_headers(${t_app} PUBLIC ${_headers})
            
            # add include dir of apps
            target_include_directories(${t_app} PUBLIC ${p}/apps)

            # setup module includes
            module_includes(${t_app} ${r_path} ${mod})

            # common compilation flags
            set_compilation(${t_app} ${app_path} ${mod})

            # link app to its own module
            target_link_libraries(${t_app} PRIVATE ${t_name})

            # format upper-case name for app name
            string(TOUPPER ${t_app} u)
            string(REPLACE "-" "_" u ${u})

            # define global name-here -> APP_NAME_HERE
            target_compile_definitions(${t_app} PRIVATE APP_${u})
            add_dependencies(${t_app} ${t_name})
            foreach(role ${r_list})
                add_dependencies(${role} ${t_app})
            endforeach()
        endforeach()

        # test products
        foreach(test ${tests})
            get_filename_component(e ${test} EXT)
            string(REGEX REPLACE "\\.[^.]*$" "" test_name ${test})
            set(test_file "${p}/tests/${test}")
            set(test_target ${test_name})
            list(APPEND test_list ${test})
            add_executable(${test_target} ${test_file})
            target_include_directories(${test_target} PUBLIC ${p}/apps)
            module_includes(${test_target} ${r_path} ${mod})
            set_compilation(${test_target} ${test_file} ${mod})
            target_link_libraries(${test_target} ${t_name})
            if (NOT TARGET test-${t_name})
                add_custom_target(test-${t_name})
                if (NOT TARGET test-all)
                    add_custom_target(test-all)
                endif()
                add_dependencies(test-all test-${t_name})
            endif()
            add_dependencies(test-${t_name} ${test_target})
            foreach(role ${r_list})
                add_dependencies(${test_target} ${role})
            endforeach()
        endforeach()

        module_includes(${t_name} ${r_path} ${mod})

        # consolidate the target property areas at the beginning or end
        set_property(TARGET   ${t_name} PROPERTY   POSITION_INDEPENDENT_CODE ON)
        set_property(TARGET   ${t_name} PROPERTY   LINKER_LANGUAGE CXX)
        set_property(TARGET   ${t_name} PROPERTY   CXX_STANDARD 23)

        if(external_repo)
            set_target_properties(${t_name} PROPERTIES EXCLUDE_FROM_ALL TRUE)
        else()
            # todo:
            # needs to iterate through all of the repos used, pkg_name for each with a target
            #target_link_libraries(${t_name} ${pkg_name}-version)
        endif()

        if(NOT dep)
            continue()
        endif()

        foreach(d ${dep})
            if (external_repo)
                set(d_t_name ${external_repo}-${d})
            else()
                set(d_t_name ${d})
            endif()
            list(FIND mods ${d} m_found)
            if(m_found GREATER -1)
                add_dependencies(${t_name} ${d_t_name})
                target_link_libraries(${t_name} ${d_t_name})
                continue()
            endif()
            string(FIND ${d} ":" index)
            if (index GREATER 0)
                string(FIND ${d} "==" iv_start)
                if (iv_start GREATER 0)
                    string(SUBSTRING ${d} 0 ${index} e_repo)
                    math(EXPR index "${index}+1")
                    math(EXPR len   "${iv_start}-${index}")
                    string(SUBSTRING ${d} ${index} ${len} e_module)
                    math(EXPR iv_start "${iv_start}+2")
                    string(SUBSTRING ${d} ${iv_start} -1 e_version)
                    list(FIND mread ${e_repo} index)
                else()
                    string(SUBSTRING ${d} 0 ${index} e_repo)
                    math(EXPR index "${index}+1")
                    string(SUBSTRING ${d} ${index} -1 e_module)
                    list(FIND mread ${e_repo} index)
                    #target_link_libraries(${t_name} ${e_repo}-version)
                endif()
                
                set(extern_r_path_src ${CMAKE_CURRENT_SOURCE_DIR}/../${e_repo})
                set(extern_r_path     ${CMAKE_CURRENT_SOURCE_DIR}/extern/${e_repo})
                
                if(NOT EXISTS ${extern_r_path})
                    file(CREATE_LINK ${extern_r_path_src} ${extern_r_path} SYMBOLIC)
                endif()

                set(pkg_path "${extern_r_path_src}/package.json")
                read_package(${pkg_path} pkg_name pkg_name_ucase pkg_version pkg_subdirs)
                set(list)
                set(extern_libs "")

                if (iv_start GREATER 0)
                    set(extern_lib     ${extern_r_path}/products/${e_version}/lib/${OS}/${ARCH}/${e_module})
                    set(extern_lib_p   ${extern_r_path}/products/${e_version}/lib/${OS}/${ARCH}/${m_pre}${e_module})
                    set(extern_include ${extern_r_path}/products/${e_version}/include)
                    if(EXISTS ${extern_lib_p}${m_dyn})
                        list(APPEND extern_libs ${extern_lib_p}${m_dyn})
                        message(STATUS "m_dyn: ${extern_lib_p}")
                    elseif(EXISTS ${extern_lib_p}${m_lib})
                        list(APPEND extern_libs ${extern_lib_p}${m_lib})
                        message(STATUS "m_lib: ${extern_lib_p}")
                    elseif(EXISTS ${extern_lib_p}${m_lib2})
                        list(APPEND extern_libs ${extern_lib_p}${m_lib2})
                        message(STATUS "m_lib2: ${extern_lib_p}")
                    else()
                        message(STATUS "shared/static library not found: ${extern_lib_p}")
                    endif()
                    if (EXISTS ${extern_include})
                        list(APPEND extern_includes "${extern_include}")
                        if(pkg_subdirs STREQUAL "true")
                            file(GLOB files "${extern_include}/*")
                            foreach (file ${files})
                                list(APPEND extern_includes "${extern_include}")
                            endforeach()
                        endif()
                    endif()
                else()
                    if (index EQUAL -1)
                        load_modules(${CMAKE_CURRENT_SOURCE_DIR}/extern/${e_repo} ${e_repo})
                        list(APPEND mread ${e_repo})
                    endif()
                    set(extern_r_path ${CMAKE_CURRENT_SOURCE_DIR}/../${e_repo})
                    set(d_t_name ${e_repo}-${e_module})
                    add_dependencies(${t_name} ${d_t_name})
                    list(APPEND extern_libs ${d_t_name})
                    list(APPEND extern_includes ${extern_r_path})
                endif()

                target_link_libraries(${t_name} ${extern_libs})
                target_include_directories(${t_name} PUBLIC ${extern_includes})
            else()
                # if not a module, try pkg-config, then standard library lookups
                if(NOT WIN32)
                    pkg_check_modules(PACKAGE QUIET ${d})
                endif()
                if(PACKAGE_FOUND)
                    target_link_libraries(${t_name}             ${PACKAGE_LINK_LIBRARIES})
                    target_include_directories(${t_name} PUBLIC ${PACKAGE_INCLUDE_DIRS})
                    target_compile_options(${t_name}     PUBLIC ${PACKAGE_CFLAGS_OTHER})
                else()
                    if(MSVC)
                        find_package(${d} CONFIG QUIET)
                        if(${d}_FOUND)
                            target_link_libraries(${t_name} ${d}::${d})
                        else()
                            target_link_libraries(${t_name} ${d})
                        endif()
                    else()
                        find_library(L_${d} ${d} QUIET)
                        if(L_${d})
                            target_link_libraries(${t_name} ${L_${d}})
                        elseif(d STREQUAL "stdc++fs")
                            target_link_libraries(${t_name} ${d})
                        endif()
                    endif()
                endif()
            endif()
        endforeach()
    endforeach()
endfunction()