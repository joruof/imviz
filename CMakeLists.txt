cmake_minimum_required(VERSION 3.0)
project(imviz)

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

# --- [ External libs ]---

set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS OFF CACHE BOOL "" FORCE)

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)

include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY "https://github.com/glfw/glfw"
    GIT_TAG "813edfa494f1f694bfe2b6d27bb731da2e277d5a"
)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY "https://github.com/ocornut/imgui"
    GIT_TAG "34d4bc620d8316f8a69ce25a75fce493fdddef5d"
)

FetchContent_Declare(
    implot
    GIT_REPOSITORY "https://github.com/epezent/implot"
    GIT_TAG "555ff688a8134bc0c602123149abe9c17d577475"
)

FetchContent_Declare(
    pybind
    GIT_REPOSITORY "https://github.com/pybind/pybind11"
    GIT_TAG "59a2ac2745d8a57ac94c6accced73620d59fb844"
)

message(STATUS "Loading glfw ...")
FetchContent_MakeAvailable(glfw)

message(STATUS "Loading imgui ...")
FetchContent_MakeAvailable(imgui)

message(STATUS "Loading implot ...")
FetchContent_MakeAvailable(implot)

message(STATUS "Loading pybind ...")
FetchContent_MakeAvailable(pybind)

message(STATUS "")
message(STATUS "All dependencies loaded.")
message(STATUS "")

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
    ${implot_SOURCE_DIR}/implot_items.cpp
    ./src/SourceSansPro.cpp
    ./src/bindings.cpp
   )

set(HEADER_FILES 
    ./src/SourceSansPro.h
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
                           ${imgui_SOURCE_DIR}
                           ${implot_SOURCE_DIR})

target_include_directories(${PY_TARGET_NAME} PUBLIC src/)

target_compile_options(${PY_TARGET_NAME} PUBLIC
                        -Wall
                        -Wextra
                        -Wpedantic
                        -Wunreachable-code
                        -std=c++17)

# Exports compile commands to .json file for vim YouCompleteMe support.

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
