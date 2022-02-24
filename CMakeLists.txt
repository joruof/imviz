cmake_minimum_required(VERSION 3.0)
project(cppimviz)

set(PY_TARGET_NAME "${PROJECT_NAME}")

# ---[ Check for OpenGL (mandatory) ]---

set(OpenGL_GL_PREFERENCE GLVND)

find_package(OpenGL QUIET)
if(OPENGL_FOUND)
    message(STATUS "Found OpenGL: " ${OPENGL_LIBRARIES})
    message(STATUS "              " ${OPENGL_INCLUDE_DIR})
else(OPENGL_FOUND)
    message(FATAL_ERROR "${ColourBoldRed}OpenGL missing.${ColourReset}")
endif()

# ---[ Check for GLEW (mandatory) ]---

find_package(GLEW QUIET)
if(GLEW_FOUND)
    message(STATUS "Found GLEW: " ${GLEW_LIBRARIES})
    message(STATUS "            " ${GLEW_INCLUDE_DIR})
else(GLEW_FOUND)
    message(FATAL_ERROR "${ColourBoldRed}GLEW missing.${ColourReset}")
endif()

# ---[ Check for GLFW3 (mandatory) ]---

find_package(glfw3 QUIET)
if(glfw3_FOUND)
    message(STATUS "Found GLFW3")
else(glfw3_FOUND)
    message(FATAL_ERROR "${ColourBoldRed}GLFW3 missing.${ColourReset}")
endif()

# ---[ Update submodules ]---
# From: https://cliutils.gitlab.io/modern-cmake/chapters/projects/submodule.html

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update ...")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

# ---[ External libs ]---

set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS OFF CACHE BOOL "" FORCE)

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)

include(FetchContent)

FetchContent_Declare(
    implot
    GIT_REPOSITORY "https://github.com/epezent/implot"
    GIT_TAG "6ee1559715fae9480fcaeb81f24d80a4d1e8c407"
)

FetchContent_Declare(
    pybind
    GIT_REPOSITORY "https://github.com/pybind/pybind11"
    GIT_TAG "59a2ac2745d8a57ac94c6accced73620d59fb844"
)

message(STATUS "Loading implot ...")
FetchContent_MakeAvailable(implot)

message(STATUS "Loading pybind ...")
FetchContent_MakeAvailable(pybind)

message(STATUS "")
message(STATUS "All dependencies loaded.")
message(STATUS "")

# Collect files.

set(SOURCE_FILES
    extern/imgui/imgui.cpp
    extern/imgui/imgui_draw.cpp
    extern/imgui/imgui_demo.cpp
    extern/imgui/imgui_widgets.cpp
    extern/imgui/imgui_tables.cpp
    extern/imgui/backends/imgui_impl_glfw.cpp
    extern/imgui/backends/imgui_impl_opengl3.cpp
    extern/imgui/misc/cpp/imgui_stdlib.cpp
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_demo.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp
    ./src/bindings.cpp
    ./src/imviz.cpp
    ./src/input.cpp
    ./src/file_dialog.cpp
    ./src/binding_helpers.cpp
    ./src/bindings_implot.cpp
    ./src/bindings_imgui.cpp
    ./src/source_sans_pro.cpp
    #./src/shader.cpp
    #./src/shader_program.cpp
   )

set(HEADER_FILES 
    ./src/imviz.hpp
    ./src/input.hpp
    ./src/file_dialog.hpp
    ./src/binding_helpers.hpp
    ./src/bindings_implot.hpp
    ./src/bindings_imgui.hpp
    ./src/source_sans_pro.hpp
    #./src/shader.hpp
    #./src/shader_program.hpp
    )

# Builds the python bindings module.

pybind11_add_module(${PY_TARGET_NAME} MODULE ${SOURCE_FILES})

target_link_libraries(${PY_TARGET_NAME} PUBLIC
                      ${OPENGL_LIBRARIES}
                      ${GLEW_LIBRARIES}
                      stdc++fs
                      pybind11::module
                      pybind11::embed
                      glfw)

target_include_directories(${PY_TARGET_NAME} SYSTEM PUBLIC
                           extern/imgui/
                           ${implot_SOURCE_DIR})

target_include_directories(${PY_TARGET_NAME} PUBLIC src/)

target_compile_options(${PY_TARGET_NAME} PUBLIC
                        -DIMGUI_USER_CONFIG="im_user_config.h"
                        -O3
                        -Wall
                        -Wextra
                        -Wpedantic
                        -Wunreachable-code
                        -std=c++17)

# Exports compile commands to .json file for vim YouCompleteMe support.

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
