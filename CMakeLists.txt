set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_minimum_required(VERSION 3.20)
project(program)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# macOS: LunarG SDK installs loader to /usr/local, headers via Homebrew to /opt/homebrew
if(APPLE)
    set(Vulkan_INCLUDE_DIR "/opt/homebrew/include" CACHE PATH "" FORCE)
    set(Vulkan_LIBRARY "/usr/local/lib/libvulkan.dylib" CACHE FILEPATH "" FORCE)
endif()

find_package(Vulkan REQUIRED)

add_subdirectory(libraries)

add_executable(program
    src/main.cpp
    src/components/camera.cpp
    src/vk_engine.cpp
    src/vk_initializers.cpp
    src/vk_images.cpp
    src/vk_descriptors.cpp
    src/vk_pipelines.cpp
)

target_include_directories(program PRIVATE src)
target_include_directories(program PRIVATE libraries/SDL/include)

target_link_libraries(program PRIVATE
    vkbootstrap
    vma
    glm
    fmt::fmt
    stb_image
    imgui
    fastgltf::fastgltf
    SDL2::SDL2
    Vulkan::Vulkan
)

# glslangValidator ships with the Vulkan SDK; FindVulkan exposes its path
set(GLSL_VALIDATOR ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE})

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/shaders/*.frag"
    "${PROJECT_SOURCE_DIR}/src/shaders/*.vert"
    "${PROJECT_SOURCE_DIR}/src/shaders/*.comp"
    )

foreach(GLSL ${GLSL_SOURCE_FILES})
  message(STATUS "BUILDING SHADER")
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_SOURCE_DIR}/src/shaders/${FILE_NAME}.spv")
  message(STATUS ${GLSL})
  message(STATUS COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV})
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(Shaders DEPENDS ${SPIRV_BINARY_FILES})
add_dependencies(program Shaders)