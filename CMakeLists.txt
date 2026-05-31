cmake_minimum_required(VERSION 3.10)
project(Lengine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# ────────────────────────────────────────────────
# Output dirs
# ────────────────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ────────────────────────────────────────────────
# Third Party include dirs
# ────────────────────────────────────────────────
set(IMGUI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/external/imgui)
set(SDL2_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/external/SDL2)
set(GL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/external/GL)
set(GLM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/external/glm)
set(ASSIMP_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/external/assimp)
set(SDL2_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)

set(PHYSX_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src/external/physx)


# ────────────────────────────────────────────────
# ImGui source files
# ────────────────────────────────────────────────


# ────────────────────────────────────────────────
# Source Collection
# ────────────────────────────────────────────────
file(GLOB_RECURSE CORE_SRC "src/core/*.cpp" "src/core/*.h")
file(GLOB_RECURSE ASSETS_SRC "src/assets/*.cpp" "src/assets/*.h")
file(GLOB_RECURSE EDITOR_SRC "src/editor/*.cpp" "src/editor/*.h")
file(GLOB_RECURSE PLATFORM_SRC "src/platform/*.cpp" "src/platform/*.h")
file(GLOB_RECURSE GRAPHICS_SRC "src/graphics/*.cpp" "src/graphics/*.h")
file(GLOB_RECURSE ANIMATION_SRC "src/animations/*.cpp" "src/animations/*.h")
file(GLOB_RECURSE PHYSICS_SRC "src/physics/*.cpp" "src/physics/*.h")
file(GLOB_RECURSE TRANSFORM_SRC "src/transform/*.cpp" "src/transform/*.h")
file(GLOB_RECURSE LOGGING_SRC "src/logging/*.cpp" "src/logging/*.h")
file(GLOB_RECURSE RESOURCES_SRC "src/resources/*.cpp" "src/resources/*.h")
file(GLOB_RECURSE SCENE_SRC "src/scene/*.cpp" "src/scene/*.h")
file(GLOB_RECURSE UTILS_SRC "src/utils/*.cpp" "src/utils/*.h")
file(GLOB_RECURSE INPUT_SRC "src/input/*.cpp" "src/input/*.h")
file(GLOB_RECURSE SCRIPTING_SRC "src/scripting/*.cpp" "src/scripting/*.h")



file(GLOB_RECURSE EXTERNAL_SRC
    "src/external/picopng.cpp"
    "src/external/picopng.h"
    "src/external/json.hpp"
    "src/external/stb_image.h"
)

# ────────────────────────────────────────────────
# Visual Studio Filters
# ────────────────────────────────────────────────
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX "" FILES
    ${ASSETS_SRC}
    ${CORE_SRC}
    ${EDITOR_SRC}
    ${PLATFORM_SRC}
    ${GRAPHICS_SRC}
    ${ANIMATION_SRC}
    ${PHYSICS_SRC}
    ${TRANSFORM_SRC}
    ${LOGGING_SRC}
    ${RESOURCES_SRC}
    ${SCENE_SRC}
    ${UTILS_SRC}
    ${EXTERNAL_SRC}
    ${INPUT_SRC}
    ${SCRIPTING_SRC}
)

# ────────────────────────────────────────────────
# Build STATIC LIBRARY
# ────────────────────────────────────────────────
add_library(${PROJECT_NAME} STATIC
    ${ASSETS_SRC}
    ${CORE_SRC}
    ${EDITOR_SRC}
    ${PLATFORM_SRC}
    ${GRAPHICS_SRC}
    ${ANIMATION_SRC}
    ${PHYSICS_SRC}
    ${TRANSFORM_SRC}
    ${LOGGING_SRC}
    ${RESOURCES_SRC}
    ${SCENE_SRC}
    ${UTILS_SRC}
    ${EXTERNAL_SRC}
    ${INPUT_SRC}
    ${SCRIPTING_SRC}

)

# ────────────────────────────────────────────────
# Include directories
# ────────────────────────────────────────────────
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/external
    ${GL_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}
    ${ASSIMP_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIR}
    ${IMGUI_INCLUDE_DIR}
    ${PHYSX_ROOT}/include
)


# ────────────────────────────────────────────────
# Link libraries
# ────────────────────────────────────────────────

set(PHYSX_LIB ${PHYSX_ROOT}/lib)

target_link_directories(${PROJECT_NAME} PUBLIC
    ${SDL2_LIB_DIR}
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    assimp-vc143-mt
    SDL2main
    SDL2
    glew32
    opengl32
    ${PHYSX_LIB}/PhysXFoundation_64.lib
    ${PHYSX_LIB}/PhysX_64.lib
    ${PHYSX_LIB}/PhysXCommon_64.lib
    ${PHYSX_LIB}/PhysXCooking_64.lib
    ${PHYSX_LIB}/PhysXExtensions_static_64.lib
    ${PHYSX_LIB}/PhysXPvdSDK_static_64.lib
    ${PHYSX_LIB}/SnippetUtils_static_64.lib

)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.json
    ${CMAKE_BINARY_DIR}/config.json
    COPYONLY
)