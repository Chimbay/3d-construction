set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_minimum_required(VERSION 3.20)
project(program)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
FetchContent_MakeAvailable(glfw)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)

# GLAD (compiled from local sources)
add_library(glad STATIC glad/src/glad.c)
target_include_directories(glad PUBLIC glad/include)

add_executable(program
    src/main.cpp
    src/display.cpp
    src/components/camera.cpp
    src/utils/utils.cpp
    src/primitives/cube/cube.cpp
    src/primitives/grid/grid.cpp
)

target_include_directories(program PRIVATE ${glfw_SOURCE_DIR}/include)
target_include_directories(program PRIVATE src)

# Windows: target_link_libraries(program PRIVATE glfw opengl32 glad glm::glm)

# macOS: cross-platform OpenGL linking via CMake's FindOpenGL module
find_package(OpenGL REQUIRED)
target_link_libraries(program PRIVATE glfw OpenGL::GL glad glm::glm)
