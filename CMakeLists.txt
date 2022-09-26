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
    GIT_TAG "v2.10.0"
)

FetchContent_Declare(
    glew
    GIT_REPOSITORY "https://github.com/Perlmint/glew-cmake"
    GIT_TAG "glew-cmake-2.2.0"
)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY "https://github.com/glfw/glfw"
    GIT_TAG "3.3-stable"
)

message(STATUS "Loading implot ...")
FetchContent_MakeAvailable(implot)

message(STATUS "Loading pybind ...")
FetchContent_MakeAvailable(pybind)

message(STATUS "Loading glew ...")
set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glew)

message(STATUS "Loading glfw ...")
FetchContent_MakeAvailable(glfw)

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
    ./src/load_image.cpp
   )

set(HEADER_FILES 
    ./src/imviz.hpp
    ./src/input.hpp
    ./src/file_dialog.hpp
    ./src/binding_helpers.hpp
    ./src/bindings_implot.hpp
    ./src/bindings_imgui.hpp
    ./src/source_sans_pro.hpp
    ./src/load_image.hpp
    ./src/stb_image.h
    )

# Builds the python bindings module.

pybind11_add_module(${PY_TARGET_NAME} MODULE ${SOURCE_FILES})

target_include_directories(${PY_TARGET_NAME} SYSTEM PUBLIC
                           extern/imgui/
                           ${implot_SOURCE_DIR})

target_include_directories(${PY_TARGET_NAME} PUBLIC src/)

IF (WIN32)
    target_compile_options(${PY_TARGET_NAME} PUBLIC
                            -DIMGUI_USER_CONFIG="im_user_config.h"
                            -Zi   # -g
                            -O2   # msvc has no -O3
                            # -Wall
                            -std:c++17)  # note colon!
    
    target_link_libraries(${PY_TARGET_NAME} PUBLIC
                            ${OPENGL_LIBRARIES}
                            libglew_static
                            # stdc++fs  # not needed with msvc
                            pybind11::module
                            pybind11::embed
                            glfw)
ELSEIF(APPLE)
    target_compile_options(${PY_TARGET_NAME} PUBLIC
                            -DIMGUI_USER_CONFIG="im_user_config.h"
                            -g
                            -O3
                            -std=c++17)

    target_link_libraries(${PY_TARGET_NAME} PUBLIC
                            ${OPENGL_LIBRARIES}
                            libglew_static
                            pybind11::module
                            #pybind11::embed  # issues with conda + ARM (Apple M1), see https://github.com/pybind/pybind11/issues/3081
                            glfw)
ELSE()
    target_compile_options(${PY_TARGET_NAME} PUBLIC
                            -DIMGUI_USER_CONFIG="im_user_config.h"
                            -g
                            -O3
                            -Wall
                            -Wextra
                            -Wpedantic
                            -Wunreachable-code
                            -std=c++17)
    target_link_libraries(${PY_TARGET_NAME} PUBLIC
                            ${OPENGL_LIBRARIES}
                            libglew_static
                            stdc++fs
                            pybind11::module
                            pybind11::embed
                            glfw)
ENDIF()

# Exports compile commands to .json file for vim YouCompleteMe support.

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
