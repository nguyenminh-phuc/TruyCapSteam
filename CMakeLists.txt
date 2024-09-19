cmake_minimum_required(VERSION 3.29)

if (POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif ()

project(TruyCapSteam)
enable_language(RC)

set(ARCH "x64")
if ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "Win32")
    set(ARCH "x86")
endif ()

add_executable(TruyCapSteam
        "extlibs/aixlog/include/aixlog.hpp"
        "extlibs/aixlog/include/aixlog_wrapper.hpp"
        "extlibs/args/include/args.hxx"
        "extlibs/WinDivert/include/windivert.h"
        "include/App.h"
        "include/ArgumentParser.h"
        "include/Localizer.h"
        "include/Logger.h"
        "include/ServiceClient.h"
        "include/ServiceServer.h"
        "include/SignalHandler.h"
        "include/Utils.h"
        "src/App.cpp"
        "src/ArgumentParser.cpp"
        "src/Localizer.cpp"
        "src/Logger.cpp"
        "src/Main.cpp"
        "src/ServiceClient.cpp"
        "src/ServiceServer.cpp"
        "src/SignalHandler.cpp"
        "src/Utils.cpp"
        "src/TruyCapSteam.rc")

target_include_directories(TruyCapSteam PRIVATE
        "extlibs/aixlog/include"
        "extlibs/args/include"
        "extlibs/WinDivert/include"
        "include")

target_compile_features(TruyCapSteam PRIVATE cxx_std_23)
target_compile_definitions(TruyCapSteam PRIVATE UNICODE NOMINMAX NOGDI WIN32_LEAN_AND_MEAN)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set_property(TARGET TruyCapSteam PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

if (MSVC)
    target_compile_options(TruyCapSteam PRIVATE /W4 /utf-8)
    target_link_options(TruyCapSteam PRIVATE /MANIFEST:NO)

    if ("${ARCH}" STREQUAL "x86")
        target_link_options(TruyCapSteam PRIVATE /SAFESEH:NO)
    endif ()

    # https://learn.microsoft.com/en-us/cpp/sanitizers/asan?view=msvc-170
    # Address sanitizer doesn't work if edit-and-continue is specified (/ZI), comment out the line that starts with set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT".
    # TODO: Build an ASan version of WinDivert
    #target_compile_options(TruyCapSteam PRIVATE $<$<CONFIG:Debug>:/fsanitize=address>)
    #target_link_options(TruyCapSteam PRIVATE $<$<CONFIG:Debug>:/fsanitize=address>)
else ()
    target_compile_options(TruyCapSteam PRIVATE -Wall -Wextra -Wpedantic)

    #target_compile_options(TruyCapSteam PRIVATE $<$<CONFIG:Debug>:-fsanitize=address>)
    #target_link_options(TruyCapSteam PRIVATE $<$<CONFIG:Debug>:-fsanitize=address>)
endif ()

add_library(WinDivert SHARED IMPORTED)
target_link_libraries(TruyCapSteam Ws2_32 WinDivert)

set_target_properties(WinDivert PROPERTIES
        IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/extlibs/WinDivert/${ARCH}/WinDivert.dll"
        IMPORTED_IMPLIB "${PROJECT_SOURCE_DIR}/extlibs/WinDivert/${ARCH}/WinDivert.lib")

add_custom_command(TARGET TruyCapSteam POST_BUILD
        COMMAND mt.exe
        -manifest \"${PROJECT_SOURCE_DIR}/src/TruyCapSteam.manifest\"
        -outputresource:"$<TARGET_FILE_DIR:TruyCapSteam>/TruyCapSteam.exe"\;\#1)

add_custom_command(TARGET TruyCapSteam POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/extlibs/WinDivert/${ARCH}/WinDivert.dll"
        $<TARGET_FILE_DIR:TruyCapSteam>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/extlibs/WinDivert/${ARCH}/WinDivert64.sys"
        $<TARGET_FILE_DIR:TruyCapSteam>)
if ("${ARCH}" STREQUAL "x86")
    add_custom_command(TARGET TruyCapSteam POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${PROJECT_SOURCE_DIR}/extlibs/WinDivert/${ARCH}/WinDivert32.sys"
            $<TARGET_FILE_DIR:TruyCapSteam>)
endif ()
