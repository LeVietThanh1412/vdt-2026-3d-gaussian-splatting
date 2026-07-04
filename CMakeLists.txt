cmake_minimum_required(VERSION 3.15)
project(3dgs_ar_pipeline CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PROJECT_THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
set(OPEN3D_SDK_DIR "")

if(DEFINED OPEN3D_ROOT AND NOT "${OPEN3D_ROOT}" STREQUAL "")
    set(OPEN3D_SDK_DIR "${OPEN3D_ROOT}")
elseif(DEFINED ENV{OPEN3D_ROOT} AND NOT "$ENV{OPEN3D_ROOT}" STREQUAL "")
    set(OPEN3D_SDK_DIR "$ENV{OPEN3D_ROOT}")
elseif(EXISTS "${PROJECT_THIRD_PARTY_DIR}/open3d-devel-linux-x86_64-cxx11-abi-0.19.0")
    set(OPEN3D_SDK_DIR "${PROJECT_THIRD_PARTY_DIR}/open3d-devel-linux-x86_64-cxx11-abi-0.19.0")
elseif(EXISTS "/home/archlinux/open3d-devel-linux-x86_64-cxx11-abi-0.19.0")
    set(OPEN3D_SDK_DIR "/home/archlinux/open3d-devel-linux-x86_64-cxx11-abi-0.19.0")
endif()

if(DEFINED ENV{VCPKG_ROOT} AND NOT "$ENV{VCPKG_ROOT}" STREQUAL "" AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "VCPKG_ROOT is set to: $ENV{VCPKG_ROOT}")
    message(STATUS "If you want vcpkg toolchain integration, pass -DCMAKE_TOOLCHAIN_FILE=$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

list(APPEND CMAKE_PREFIX_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/share"
    "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/share"
)

find_package(CLI11 REQUIRED CONFIG)

if(OPEN3D_SDK_DIR STREQUAL "")
    message(FATAL_ERROR "Open3D SDK not found. Set OPEN3D_ROOT or place the SDK under third_party/open3d-devel-linux-x86_64-cxx11-abi-0.19.0")
endif()

find_package(Open3D REQUIRED CONFIG
    PATHS "${OPEN3D_SDK_DIR}/lib/cmake/Open3D"
    NO_DEFAULT_PATH)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_executable(gsplat_ar
    src/main.cpp
    src/core/mesh_converter.cpp
    src/core/stats_printer.cpp
    src/io/ply_reader.cpp
    src/io/glb_exporter.cpp
    src/processor/gsplat_filter.cpp
    src/processor/mesh_generator.cpp
    src/processor/mesh_optimizer.cpp
    src/processor/ar_extractor.cpp
    src/pipeline/optimization_pipeline.cpp
)

target_include_directories(gsplat_ar PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/happly
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinygltf
)

target_link_libraries(gsplat_ar PRIVATE
    CLI11::CLI11
    Open3D::Open3D
)

set_target_properties(gsplat_ar PROPERTIES
    BUILD_RPATH "${OPEN3D_SDK_DIR}/lib"
    INSTALL_RPATH "${OPEN3D_SDK_DIR}/lib"
)

enable_testing()

add_executable(test_io
    tests/test_io.cpp
    src/core/mesh_converter.cpp
    src/core/stats_printer.cpp
    src/io/ply_reader.cpp
    src/io/glb_exporter.cpp
    src/processor/gsplat_filter.cpp
    src/processor/mesh_generator.cpp
    src/processor/mesh_optimizer.cpp
    src/processor/ar_extractor.cpp
    src/pipeline/optimization_pipeline.cpp
)
target_include_directories(test_io PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/happly
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinygltf
)
target_link_libraries(test_io PRIVATE CLI11::CLI11 Open3D::Open3D)
add_test(NAME test_io COMMAND test_io)

add_executable(test_filter
    tests/test_filter.cpp
    src/core/mesh_converter.cpp
    src/core/stats_printer.cpp
    src/io/ply_reader.cpp
    src/io/glb_exporter.cpp
    src/processor/gsplat_filter.cpp
    src/processor/mesh_generator.cpp
    src/processor/mesh_optimizer.cpp
    src/processor/ar_extractor.cpp
    src/pipeline/optimization_pipeline.cpp
)
target_include_directories(test_filter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/happly
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinygltf
)
target_link_libraries(test_filter PRIVATE CLI11::CLI11 Open3D::Open3D)
add_test(NAME test_filter COMMAND test_filter)