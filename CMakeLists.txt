cmake_minimum_required(VERSION 3.10)

# Project declaration (THIS WAS MISSING)
project(SmokeSimulator C)

set(CMAKE_C_STANDARD 11)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# Source files
set(SOURCES
    Src/smoke.c
    Src/glad.c
    Src/Shader/fluid.vs
    Src/Shader/fluid.fs
)

# Create executable
add_executable(smoke_simulator ${SOURCES})

# Include directories
target_include_directories(smoke_simulator PRIVATE
    ${CMAKE_SOURCE_DIR}/include/GLAD
    ${CMAKE_SOURCE_DIR}/include/GLFW
    ${CMAKE_SOURCE_DIR}/include/KHR
    ${CMAKE_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(smoke_simulator
    OpenGL::GL
    glfw
)

# Windows-specific settings
if(WIN32)
    target_link_libraries(smoke_simulator opengl32)
endif()