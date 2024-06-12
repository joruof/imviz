cmake_minimum_required(VERSION 3.0)
project(cppimviz)

# Exports compile commands to .json file for vim YouCompleteMe support.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Sometimes helps to find the right python version.
set(PYBIND11_FINDPYTHON ON)

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

# ---[ External libs ]---

set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS OFF CACHE BOOL "" FORCE)

include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY "https://github.com/ocornut/imgui"
    GIT_TAG "v1.90.6-docking"
)

message(STATUS "Loading imgui ...")
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
    implot
    GIT_REPOSITORY "https://github.com/epezent/implot"
    GIT_TAG "v0.16"
)

message(STATUS "Loading implot ...")
FetchContent_MakeAvailable(implot)

FetchContent_Declare(
    pybind
    GIT_REPOSITORY "https://github.com/pybind/pybind11"
    GIT_TAG "v2.11.1"
)

message(STATUS "Loading pybind ...")
FetchContent_MakeAvailable(pybind)

if(WIN32 OR APPLE)

    FetchContent_Declare(
        glew
        GIT_REPOSITORY "https://github.com/Perlmint/glew-cmake"
        GIT_TAG "glew-cmake-2.2.0"
    )

    message(STATUS "Loading glew ...")
    set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glew)

else()

    find_package(GLEW QUIET)
    if(GLEW_FOUND)
        message(STATUS "Found GLEW: " ${GLEW_LIBRARIES})
        message(STATUS "            " ${GLEW_INCLUDE_DIR})
    else(GLEW_FOUND)
        message(FATAL_ERROR "${ColourBoldRed}GLEW missing.${ColourReset}")
    endif()

endif()

if(WIN32 OR APPLE) 

    set(GLFW_BUILD_EXAMPLES OFF)
    set(GLFW_BUILD_TESTS OFF)
    set(GLFW_BUILD_DOCS OFF)

    FetchContent_Declare(
        glfw
        GIT_REPOSITORY "https://github.com/glfw/glfw"
        GIT_TAG "3.3-stable"
    )

    message(STATUS "Loading glfw ...")
    FetchContent_MakeAvailable(glfw)

else()

    find_package(glfw3 QUIET)
    if(glfw3_FOUND)
        message(STATUS "Found GLFW3")
    else(glfw3_FOUND)
        message(FATAL_ERROR "${ColourBoldRed}GLFW3 missing.${ColourReset}")
    endif()

endif()

# Collect files.

set(SOURCE_FILES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_demo.cpp
    ./src/bindings.cpp
    ./src/imviz.cpp
    ./src/input.cpp
    ./src/file_dialog.cpp
    ./src/binding_helpers.cpp
    ./src/bindings_implot.cpp
    ./src/bindings_imgui.cpp
    ./src/source_sans_pro.cpp
    ./src/fa_solid_900.cpp
    ./src/implot_ext.cpp
   )

set(HEADER_FILES 
    ./src/imviz.hpp
    ./src/input.hpp
    ./src/file_dialog.hpp
    ./src/binding_helpers.hpp
    ./src/bindings_implot.hpp
    ./src/bindings_imgui.hpp
    ./src/source_sans_pro.hpp
    ./src/fa_solid_900.hpp
    )

# Builds the python bindings module.

pybind11_add_module(${PY_TARGET_NAME} MODULE ${SOURCE_FILES})

target_include_directories(${PY_TARGET_NAME} SYSTEM PUBLIC
                           ${imgui_SOURCE_DIR}
                           ${implot_SOURCE_DIR})

target_include_directories(${PY_TARGET_NAME} PUBLIC src/)

if(WIN32)

    target_compile_options(${PY_TARGET_NAME} PUBLIC
                            -DIMGUI_USER_CONFIG="im_user_config.h"
                            -Zi   # -g
                            -O2   # msvc has no -O3
                            -std:c++17)  # note colon!
    
    target_link_libraries(${PY_TARGET_NAME} PUBLIC
                            ${OPENGL_LIBRARIES}
                            libglew_static
                            pybind11::module
                            glfw)

elseif(APPLE)

    target_compile_options(${PY_TARGET_NAME} PUBLIC
                            -DIMGUI_USER_CONFIG="im_user_config.h"
                            -g
                            -O3
                            -std=c++17)

    target_link_libraries(${PY_TARGET_NAME} PUBLIC
                            ${OPENGL_LIBRARIES}
                            libglew_static
                            pybind11::module
                            glfw)

else()

    target_compile_options(${PY_TARGET_NAME} PUBLIC
                            -DIMGUI_USER_CONFIG="im_user_config.h"
                            -g
                            -O3
                            -ffast-math
                            -Wall
                            -Wextra
                            -Wpedantic
                            -Wunreachable-code
                            -std=c++17)

    target_link_libraries(${PY_TARGET_NAME} PUBLIC
                            ${OPENGL_LIBRARIES}
                            ${OPENGL_egl_LIBRARY}
                            ${GLEW_LIBRARIES}
                            stdc++fs
                            pybind11::module
                            glfw)

endif()
