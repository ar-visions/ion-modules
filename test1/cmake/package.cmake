cmake_minimum_required(VERSION 3.3)

if (DEFINED PackageGuard)
    return()
endif()
set(PackageGuard yes)

function(read_package location package_name package_name_ucase subdirs)
    #if(exists ${location})
    file(READ ${location} package_contents)
    if(NOT package_contents)
        message(FATAL_ERROR "package.json not in package root")
    endif()
    sbeParseJson(package package_contents)
    set(${package_name} ${package.name} PARENT_SCOPE)
    if(subdirs)
        set(${subdirs} ${package.options.include_subdirs} PARENT_SCOPE)
    endif()
    string(TOUPPER ${package.name} pkg_name_ucase)
    string(REPLACE "-" "_" pkg_name_ucase ${pkg_name_ucase})
    if(package_name_ucase)
        set(${package_name_ucase} ${pkg_name_ucase} PARENT_SCOPE)
    endif()
    sbeClearJson(package)
endfunction()
