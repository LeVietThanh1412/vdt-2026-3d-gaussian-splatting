#include "io/ply_reader.hpp"

#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>

// ──────────────────────────────────────────────────────────
//  Minimal unit tests for PlyReader (no GTest dependency).
//  Compile:  g++ -std=c++17 -I include -I third_party/happly
//            tests/test_io.cpp src/io/ply_reader.cpp -o test_io
// ──────────────────────────────────────────────────────────

/// Write a tiny ASCII PLY with 3 vertices for testing.
static std::string write_test_ply(const std::string& path) {
    std::ofstream f(path);
    f << "ply\n"
      << "format ascii 1.0\n"
      << "element vertex 3\n"
      << "property float x\n"
      << "property float y\n"
      << "property float z\n"
      << "property float opacity\n"
      << "property float scale_0\n"
      << "property float scale_1\n"
      << "property float scale_2\n"
      << "property float rot_0\n"
      << "property float rot_1\n"
      << "property float rot_2\n"
      << "property float rot_3\n"
      << "property float f_dc_0\n"
      << "property float f_dc_1\n"
      << "property float f_dc_2\n"
      << "end_header\n"
      << "1.0 2.0 3.0  5.0  0.0 0.0 0.0  1.0 0.0 0.0 0.0  0.5 0.5 0.5\n"
      << "4.0 5.0 6.0  -5.0 0.0 0.0 0.0  1.0 0.0 0.0 0.0  0.0 0.0 0.0\n"
      << "7.0 8.0 9.0  0.0  0.0 0.0 0.0  1.0 0.0 0.0 0.0  1.0 1.0 1.0\n";
    return path;
}

static void test_read_count() {
    std::string path = "data/output/_test_io_temp.ply";
    write_test_ply(path);

    PlyReader reader;
    auto splats = reader.read(path);
    assert(splats.size() == 3);
    std::cout << "[PASS] test_read_count: read 3 splats" << std::endl;

    // Remove temp file
    std::remove(path.c_str());
}

static void test_positions() {
    std::string path = "data/output/_test_io_pos.ply";
    write_test_ply(path);

    PlyReader reader;
    auto splats = reader.read(path);

    // First vertex should be at (1, 2, 3)
    assert(std::fabs(splats[0].x - 1.0f) < 1e-4f);
    assert(std::fabs(splats[0].y - 2.0f) < 1e-4f);
    assert(std::fabs(splats[0].z - 3.0f) < 1e-4f);
    std::cout << "[PASS] test_positions: vertex 0 is (1,2,3)" << std::endl;

    std::remove(path.c_str());
}

static void test_opacity_sigmoid() {
    std::string path = "data/output/_test_io_op.ply";
    write_test_ply(path);

    PlyReader reader;
    auto splats = reader.read(path);

    // Vertex 0 has raw opacity = 5.0 → sigmoid ≈ 0.9933
    assert(splats[0].opacity > 0.99f);
    // Vertex 1 has raw opacity = -5.0 → sigmoid ≈ 0.0067
    assert(splats[1].opacity < 0.01f);
    // Vertex 2 has raw opacity = 0.0 → sigmoid = 0.5
    assert(std::fabs(splats[2].opacity - 0.5f) < 0.01f);

    std::cout << "[PASS] test_opacity_sigmoid: sigmoid activation correct" << std::endl;

    std::remove(path.c_str());
}

int main() {
    std::cout << "===== IO Tests =====" << std::endl;
    test_read_count();
    test_positions();
    test_opacity_sigmoid();
    std::cout << "All IO tests passed." << std::endl;
    return 0;
}
