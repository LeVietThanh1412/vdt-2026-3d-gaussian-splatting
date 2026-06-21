cmake_minimum_required(VERSION 3.15)
project(3dgs_ar_pipeline CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Dependencies ──────────────────────────────────────────
find_package(CLI11 REQUIRED)

# Open3D – prefer the pre-built SDK if available
set(OPEN3D_INSTALL_DIR "$ENV{OPEN3D_ROOT}")
if(NOT OPEN3D_INSTALL_DIR OR OPEN3D_INSTALL_DIR STREQUAL "")
    set(OPEN3D_INSTALL_DIR "/home/archlinux/open3d-devel-linux-x86_64-cxx11-abi-0.19.0")
endif()
find_package(Open3D REQUIRED CONFIG
    PATHS "${OPEN3D_INSTALL_DIR}/lib/cmake/Open3D" NO_DEFAULT_PATH)

# ── Include paths ─────────────────────────────────────────
include_directories(
    include
    third_party/happly
    third_party/tinygltf
)

# ── Library source files (everything except main.cpp) ─────
# Listed explicitly to avoid GLOB_RECURSE pitfalls when adding new files.
set(LIB_SOURCES
    # Core utilities
    src/core/mesh_converter.cpp
    src/core/stats_printer.cpp
    # IO
    src/io/ply_reader.cpp
    src/io/glb_exporter.cpp
    # Processors
    src/processor/gsplat_filter.cpp
    src/processor/mesh_generator.cpp
    src/processor/mesh_optimizer.cpp
    src/processor/ar_extractor.cpp
    # Pipeline orchestrator
    src/pipeline/optimization_pipeline.cpp
)

# ── Main executable ──────────────────────────────────────
add_executable(gsplat_ar
    src/main.cpp
    ${LIB_SOURCES}
)
target_link_libraries(gsplat_ar PRIVATE CLI11::CLI11 Open3D::Open3D)

# Ensure Open3D RPATH is set correctly for runtime
set_target_properties(gsplat_ar PROPERTIES
    INSTALL_RPATH "${OPEN3D_INSTALL_DIR}/lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# ── Tests ─────────────────────────────────────────────────
# Always build tests — they use simple assert(), no GTest required.
enable_testing()

add_executable(test_io
    tests/test_io.cpp
    ${LIB_SOURCES}
)
target_link_libraries(test_io PRIVATE CLI11::CLI11 Open3D::Open3D)
add_test(NAME test_io COMMAND test_io)

add_executable(test_filter
    tests/test_filter.cpp
    ${LIB_SOURCES}
)
target_link_libraries(test_filter PRIVATE CLI11::CLI11 Open3D::Open3D)
add_test(NAME test_filter COMMAND test_filter)