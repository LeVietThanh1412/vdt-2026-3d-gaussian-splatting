#include "core/stats_printer.hpp"

#include <iomanip>
#include <iostream>

void stats_printer::print(const PipelineStats& s) {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════╗\n";
    std::cout << "║              PIPELINE REPORT                    ║\n";
    std::cout << "╠══════════════════════════════════════════════════╣\n";
    std::cout << "║  Input splats        : " << std::setw(10) << s.input_splats
              << "               ║\n";
    std::cout << "║  After filtering     : " << std::setw(10) << s.filtered_splats
              << "               ║\n";
    std::cout << "║  Mesh vertices       : " << std::setw(10) << s.mesh_vertices
              << "               ║\n";
    std::cout << "║  Mesh triangles      : " << std::setw(10) << s.mesh_triangles
              << "               ║\n";
    std::cout << "║  Optimized triangles : " << std::setw(10) << s.optimized_triangles
              << "               ║\n";
    std::cout << "║  Occlusion faces     : " << std::setw(10) << s.occlusion_faces
              << "               ║\n";
    std::cout << "║  Navigation faces    : " << std::setw(10) << s.nav_faces
              << "               ║\n";
    std::cout << "╠══════════════════════════════════════════════════╣\n";
    std::cout << "║  Time - Read PLY     : " << std::setw(10) << std::fixed
              << std::setprecision(1) << s.time_read_ms << " ms          ║\n";
    std::cout << "║  Time - Filter       : " << std::setw(10) << s.time_filter_ms
              << " ms          ║\n";
    std::cout << "║  Time - Mesh Gen     : " << std::setw(10) << s.time_mesh_ms
              << " ms          ║\n";
    std::cout << "║  Time - Optimize     : " << std::setw(10) << s.time_optimize_ms
              << " ms          ║\n";
    std::cout << "║  Time - AR Extract   : " << std::setw(10) << s.time_extract_ms
              << " ms          ║\n";
    std::cout << "║  Time - GLB Export   : " << std::setw(10) << s.time_export_ms
              << " ms          ║\n";
    std::cout << "║  ─────────────────────────────────────          ║\n";
    std::cout << "║  TOTAL               : " << std::setw(10) << s.time_total_ms
              << " ms          ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n";
}
