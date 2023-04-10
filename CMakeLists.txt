cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

set(VERSION 1.0)
set(PROJECT_NAME VK-RT)

set(VCPKG_TARGET_ARCHITECTURE "x64")
set(VCPKG_CRT_LINKAGE "static")
set(VCPKG_LIBRARY_LINKAGE "static")

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
    include/Result.h
    include/VulkanBuffer.h
    include/Window.h
    include/Context.h
    include/Swapchain.h
    include/RayTracingPipeline.h
    include/Scene.h
    include/Model.h
    include/Object.h
    include/Renderer.h
    include/Camera.h
    include/InputManager.h
    include/Light.h
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
    src/RayTracingPipeline.cpp
    src/Scene.cpp
    src/Model.cpp
    src/Object.cpp
    src/Renderer.cpp
    src/Camera.cpp
    src/InputManager.cpp
    src/Light.cpp
)

set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")

set(SHADERS
    raytrace.rgen
    raytrace.rchit
    raytrace.rmiss
    raytraceShadow.rmiss
)

if(WIN32)
    set(PLATFORM_SPECIFIC_FILES
        include/ShaderResources.h
        include/ShaderResources.rc
    )
elseif(UNIX)
    set(PLATFORM_SPECIFIC_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/incbin/incbin.h
        ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/incbin/incbin.c
    )
endif()

add_executable(${PROJECT_NAME}
    ${HEADERS}
    ${SOURCE}
    ${PLATFORM_SPECIFIC_FILES}
)

if(WIN32)
    add_compile_definitions(VKRT_PLATFORM_WINDOWS)
elseif(UNIX)
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

add_compile_definitions(GLM_FORCE_RADIANS)
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)
find_package(glm CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)

find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
target_include_directories(${PROJECT_NAME} PRIVATE ${TINYGLTF_INCLUDE_DIRS})

find_package(nlohmann_json CONFIG REQUIRED)
find_path(NLOHMANN_JSON_INCLUDE nlohmann/json.hpp)
include_directories(${NLOHMANN_JSON_INCLUDE})

if(UNIX)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/incbin)
endif()

# Add shader compilation build step
if(WIN32)
    set(SHADER_COMPILER_PATH "$ENV{VULKAN_SDK}/Bin/glslc.exe")
elseif(UNIX)
    set(SHADER_COMPILER_PATH "$ENV{VULKAN_SDK}/bin/glslc")
endif()
set(SHADER_DEPENDENCIES "${PROJECT_NAME}Shaders")
add_dependencies(${PROJECT_NAME} ${SHADER_DEPENDENCIES})

foreach(SHADER ${SHADERS})
    set(COMPILED_SHADER "${SHADER}.spv")
    set(SRC_SHADER_PATH "${SHADER_SRC_DIR}/${SHADER}")
    set(COMPILED_SHADER_PATH "${SHADER_SRC_DIR}/${COMPILED_SHADER}")
    list(APPEND COMPILED_SHADERS ${COMPILED_SHADER_PATH})
    add_custom_command(
        OUTPUT ${COMPILED_SHADER_PATH}
        COMMAND ${SHADER_COMPILER_PATH} ${SRC_SHADER_PATH} -o ${COMPILED_SHADER_PATH} --target-env=vulkan1.2
        DEPENDS ${SRC_SHADER_PATH}
    )
endforeach()

add_custom_target(${SHADER_DEPENDENCIES} DEPENDS ${COMPILED_SHADERS})
foreach(SHADER ${SHADERS})
    set(COMPILED_SHADER "${SHADER}.spv")
    set(COMPILED_SHADER_PATH "${SHADER_SRC_DIR}/${COMPILED_SHADER}")
    add_custom_command(
        TARGET ${SHADER_DEPENDENCIES} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${COMPILED_SHADER_PATH}
                ${CMAKE_CURRENT_BINARY_DIR}/${COMPILED_SHADER})
endforeach()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
