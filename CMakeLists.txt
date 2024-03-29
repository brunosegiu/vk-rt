cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

set(VERSION 1.0)
set(PROJECT_NAME VK-RT)

set(VCPKG_TARGET_ARCHITECTURE "x64")
set(VCPKG_CRT_LINKAGE "static")
set(VCPKG_LIBRARY_LINKAGE "static")

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(VCPKG_FEATURE_FLAGS manifests)

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
    include/Scene.h
    include/Model.h
    include/Object.h
    include/Renderer.h
    include/Camera.h
    include/InputManager.h
    include/Light.h
    include/Texture.h
    include/Material.h
    include/Mesh.h
    include/ProbeGrid.h
    include/Pipeline.h
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
    src/Scene.cpp
    src/Model.cpp
    src/Object.cpp
    src/Renderer.cpp
    src/Camera.cpp
    src/InputManager.cpp
    src/Light.cpp
    src/Texture.cpp
    src/Material.cpp
    src/Mesh.cpp
    src/ProbeGrid.cpp
    src/Pipeline.cpp
)

set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")

set(SHADER_HEADERS 
    definitions.glsl
    proceduralSky.glsl
    equirectangularProjection.glsl
)

set(SHADERS
    raytrace.rgen
    raytrace.rchit
    raytrace.rmiss
    raytraceShadow.rmiss
    raytraceProbe.rgen
    raytraceProbe.rchit
    raytraceProbe.rmiss
    raytraceProbeShadow.rmiss
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

set(ALL_SHADERS ${SHADERS} ${SHADER_HEADERS})
foreach(SHADER ${ALL_SHADERS})
    set(RELATIVE_SHADER_PATH "shaders/${SHADER}")
    list(APPEND RELATIVE_SHADER_PATHS ${RELATIVE_SHADER_PATH})
endforeach()
source_group("Shaders" FILES ${RELATIVE_SHADER_PATHS})

add_executable(${PROJECT_NAME}
    ${HEADERS}
    ${SOURCE}
    ${RELATIVE_SHADER_PATHS}
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

# Find and link Vulkan
add_compile_definitions(VULKAN_HPP_NO_EXCEPTIONS)
find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
include_directories(${Vulkan_INCLUDE_DIRS})

# Find and link GLFW
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

# Find and link GLM
add_compile_definitions(GLM_FORCE_RADIANS)
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)
find_package(glm CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)

# Find and link TinyGLTF
add_compile_definitions(TINYGLTF_IMPLEMENTATION)
add_compile_definitions(STB_IMAGE_IMPLEMENTATION)
add_compile_definitions(STB_IMAGE_WRITE_IMPLEMENTATION)
add_compile_definitions(TINYGLTF_NO_INCLUDE_JSON)
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
target_include_directories(${PROJECT_NAME} PRIVATE ${TINYGLTF_INCLUDE_DIRS})

# Find and link JSON (TinyGLTF dependency)
find_package(nlohmann_json CONFIG REQUIRED)
find_path(NLOHMANN_JSON_INCLUDE nlohmann/json.hpp)
include_directories(${NLOHMANN_JSON_INCLUDE})

# Find and link SDL2
find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)

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

foreach(SHADER ${SHADER_HEADERS})
    list(APPEND SHADER_HEADER_PATHS "${SHADER_SRC_DIR}/${SHADER}")
endforeach()

foreach(SHADER ${SHADERS})
    set(COMPILED_SHADER "${SHADER}.spv")
    set(SRC_SHADER_PATH "${SHADER_SRC_DIR}/${SHADER}")
    set(COMPILED_SHADER_PATH "${CMAKE_CURRENT_BINARY_DIR}/${COMPILED_SHADER}")
    list(APPEND COMPILED_SHADERS ${COMPILED_SHADER_PATH})
    set(DEPENDENCY_LIST ${SHADER_HEADER_PATHS})
    list(APPEND DEPENDENCY_LIST ${SRC_SHADER_PATH})
    add_custom_command(
        OUTPUT ${COMPILED_SHADER_PATH}
        COMMAND ${SHADER_COMPILER_PATH} -o ${COMPILED_SHADER_PATH} --target-env=vulkan1.2 -Werror ${SRC_SHADER_PATH}
        DEPENDS ${DEPENDENCY_LIST}
    )
endforeach()

add_custom_target(${SHADER_DEPENDENCIES} DEPENDS ${COMPILED_SHADERS})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
