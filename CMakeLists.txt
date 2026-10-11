cmake_minimum_required(VERSION 3.16)
project(webwin C CXX)

option(WEBWIN_PTHREADS "Build for Emscripten pthreads (SharedArrayBuffer); OFF builds the cooperative fiber scheduler" OFF)

add_library(webwin STATIC
    src/win_kernel.cpp
    src/web_fibers.cpp
    src/web_fs.cpp
    src/sdl_window.cpp
    src/msvc_rand.cpp
    src/msvc_crt.cpp
    src/process.cpp
)
set_target_properties(webwin PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

# Sources include <Windows.h> as well as <windows.h>; on a case-sensitive filesystem the
# capitalised name needs its own file.
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/case/Windows.h" "#include \"windows.h\"\n")

target_include_directories(webwin PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}/case"
)
target_compile_options(webwin PUBLIC
    "$<$<COMPILE_LANGUAGE:CXX>:-include;${CMAKE_CURRENT_SOURCE_DIR}/include/compat/msvc_compat.h>"
    "$<$<COMPILE_LANGUAGE:C>:-D__cdecl=;-D__stdcall=;-D__fastcall=;-D__int8=char;-D__int16=short;-D__int32=int>"
)

if (EMSCRIPTEN)
    target_compile_options(webwin PUBLIC -sUSE_SDL=2)
    target_link_options(webwin PUBLIC -sUSE_SDL=2)
    if (WEBWIN_PTHREADS)
        target_compile_options(webwin PUBLIC -pthread)
        target_link_options(webwin PUBLIC -pthread)
    endif()
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WEBWIN_SDL2 REQUIRED sdl2)
    target_include_directories(webwin PRIVATE ${WEBWIN_SDL2_INCLUDE_DIRS})
    target_link_libraries(webwin PUBLIC ${WEBWIN_SDL2_LIBRARIES} pthread)
endif()

# Files the page must serve next to the wasm; the consumer copies them.
set_target_properties(webwin PROPERTIES WEB_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/web/web_fs.js;${CMAKE_CURRENT_SOURCE_DIR}/web/coi-serviceworker.js")
