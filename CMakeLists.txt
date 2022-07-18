cmake_minimum_required(VERSION 3.13.0)
project(gathering-simulator VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
OPTION(GATHERING_BUILD_SAMPLES
    "ON to build the examples." 
    ON)
OPTION(GATHERING_CREATE_DOCS
    "ON to create the docs." 
    OFF)
OPTION(GATHERING_DEBUGPRINTS
    "ON to enable debug prints." 
    ON)
OPTION(GATHERING_AUTO_HEADLESS
    "ON to enable headless simulation when manually closing a window." 
    ON)
OPTION(GATHERING_PYBIND
    "ON to build a MODULE library for python with PyBind11." 
    ON)

# GLM library
include_directories(external/glm)
# GLFW library
add_subdirectory(external/glfw)
# imgui library
add_subdirectory(external)
# pybind11 library
if(GATHERING_PYBIND)
    add_definitions(-DGATHERING_PYBIND)
    add_subdirectory(external/pybind11)
    add_subdirectory(external/eigen)
endif()

if(GATHERING_PYBIND)
    pybind11_add_module(gathering MODULE include/gathering/pybind.cpp)
else()
    add_library(gathering STATIC)
endif()
set_target_properties(gathering PROPERTIES PROJECT_SOURCE_DIR ${PROJECT_SOURCE_DIR})

# macro for copying resource files
macro(copy_resources target)
    get_target_property(source_dir gathering PROJECT_SOURCE_DIR) # to be independent of the calling target/project
    add_custom_command(
        TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${source_dir}/resources/shader $<TARGET_FILE_DIR:${target}>/shader
        VERBATIM)
endmacro()

# Examples
if(GATHERING_BUILD_SAMPLES AND NOT GATHERING_PYBIND)
    add_subdirectory(./examples)
endif()

# docs
if(GATHERING_CREATE_DOCS)
    find_package(Doxygen REQUIRED)
    SET(doxyfile "${CMAKE_CURRENT_BINARY_DIR}/docs/doxyconfig")
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/doxyconfig
        ${CMAKE_CURRENT_BINARY_DIR}/docs/doxyconfig
        @ONLY
    )
    add_custom_target(
        doc ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
        COMMENT "Generating documentation with Doxygen"
        VERBATIM
    )
endif()

# debug prints
if(GATHERING_DEBUGPRINTS)
    add_definitions(-DGATHERING_DEBUGPRINTS)
endif()

# auto headless
if(GATHERING_AUTO_HEADLESS)
    add_definitions(-DGATHERING_AUTO_HEADLESS)
endif()

# external source files
set(external_files
    external/glad/src/glad.c
)

# source files
target_sources(gathering
    PRIVATE
        src/simulation.cpp
        src/opengl_widget.cpp
        src/opengl_primitives.cpp
        src/opengl_toolkit.cpp
        src/scene.cpp
        src/particle.cpp
        src/meta.hpp
        src/container.cpp
        ${external_files}
)

target_include_directories(gathering
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        external/glad/include
        external/stb
        external/eigen
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gathering PRIVATE glfw imgui)

if(UNIX)
    set_target_properties(gathering
 PROPERTIES COMPILE_FLAGS -pthread LINK_FLAGS -pthread)
    target_link_libraries(gathering ${CMAKE_DL_LIBS})
endif()
