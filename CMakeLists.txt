cmake_minimum_required(VERSION 3.26 FATAL_ERROR)

set(VERSION 1.0)
set(PROJECT_NAME VK-RT)

if (MSVC)
    set(VCPKG_TARGET_TRIPLET "x64-windows")
endif()

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(VCPKG_FEATURE_FLAGS manifests)

set(CMAKE_CONFIGURATION_TYPES Debug;Release)

project(${PROJECT_NAME} VERSION ${VERSION} LANGUAGES CXX)

set(HEADERS
    include/Instance.h
    include/Device.h
    include/RefCountPtr.h
    include/Macros.h
    include/DebugUtils.h
    include/ResourceLoader.h
    include/VulkanBase.h
    include/ShaderResources.h
    include/ShaderResources.rc
    include/Result.h
    include/VulkanBuffer.h
    include/Window.h
    include/Context.h
    include/Swapchain.h
)

set(SOURCE
    src/ResourceLoader.cpp
    src/Device.cpp
    src/Instance.cpp
    src/RefCountPtr.cpp
    src/VulkanBuffer.cpp
    src/main.cpp
    src/Window.cpp
    src/Context.cpp
    src/Swapchain.cpp
)

set(SHADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/opaque.rgen
)

add_executable(${PROJECT_NAME}
    ${HEADERS}
    ${SOURCE}
)

if(MSVC)
    add_compile_definitions(VKRT_PLATFORM_WINDOWS)
elseif(LINUX)
    add_compile_definitions(VKRT_PLATFORM_LINUX)
endif()
add_compile_definitions(NOMINMAX)

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${HEADERS}")

include_directories(include)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

# Find Vulkan SDK
add_compile_definitions(VULKAN_HPP_NO_EXCEPTIONS)
find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
include_directories(${Vulkan_INCLUDE_DIRS})

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

find_package(glm CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)

# Add shader compilation build step
set(SHADER_COMPILER_PATH "$ENV{VULKAN_SDK}/Bin/glslc.exe")

set(COMPILED_SHADER "${SHADERS}.spv")
add_custom_command(
    OUTPUT ${COMPILED_SHADER}
    COMMAND ${SHADER_COMPILER_PATH} ${SHADERS} -o ${COMPILED_SHADER}
    DEPENDS ${SHADERS}
)
set(SHADER_DEPENDENCIES "${PROJECT_NAME}Shaders")
add_custom_target(${SHADER_DEPENDENCIES} DEPENDS ${COMPILED_SHADER})
add_dependencies(${PROJECT_NAME} ${SHADER_DEPENDENCIES})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
