cmake_minimum_required(VERSION 3.0)
project(implot)

set(PY_TARGET_NAME "py${PROJECT_NAME}")

# ---[ Check for OpenGL (mandatory) ]---

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

# --- [ External libs ]---

set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS OFF CACHE BOOL "" FORCE)
set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)

add_subdirectory(extern/pybind_pybind11 EXCLUDE_FROM_ALL)

# Collect files.

set(SOURCE_FILES
    ./extern/ocornut_imgui/imgui.cpp
    ./extern/ocornut_imgui/imgui_draw.cpp
    ./extern/ocornut_imgui/imgui_demo.cpp
    ./extern/ocornut_imgui/imgui_impl_glfw.cpp
    ./extern/ocornut_imgui/imgui_widgets.cpp
    ./extern/ocornut_imgui/imgui_impl_opengl3.cpp
    ./extern/ocornut_imgui/imgui_stdlib.cpp
    ./extern/ocornut_imgui/implot.cpp
    ./extern/ocornut_imgui/implot_demo.cpp
    ./src/bindings.cpp
   )

set(HEADER_FILES )

# Builds the python bindings module.

pybind11_add_module(${PY_TARGET_NAME} MODULE ${SOURCE_FILES})

set_target_properties(${PY_TARGET_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/python/build")

target_link_libraries(${PY_TARGET_NAME} PUBLIC
                      ${OPENGL_LIBRARIES}
                      ${GLEW_LIBRARIES}
                      stdc++fs
                      pybind11::module
                      pybind11::embed
                      glfw)

target_include_directories(${PY_TARGET_NAME}
                           PUBLIC src/
                           PUBLIC extern/
                           PUBLIC ${OPENGL_INCLUDE_DIR})

target_compile_options(${PY_TARGET_NAME} PUBLIC
                        -Wall
                        -Wextra
                        -Wpedantic
                        -Wunreachable-code
                        -std=c++17)

# Exports compile commands to .json file for vim YouCompleteMe support.

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
