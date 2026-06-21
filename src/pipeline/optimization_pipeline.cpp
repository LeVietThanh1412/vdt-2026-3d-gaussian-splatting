#include "pipeline/optimization_pipeline.hpp"
#include "core/stats_printer.hpp"
#include "io/ply_reader.hpp"
#include "io/glb_exporter.hpp"
#include "processor/gsplat_filter.hpp"
#include "processor/mesh_generator.hpp"
#include "processor/mesh_optimizer.hpp"
#include "processor/ar_extractor.hpp"

#include <chrono>
#include <iostream>

// ──────────────────────────────────────────────────────────
//  Trình đo thời gian thực thi (ms)
// ──────────────────────────────────────────────────────────

using Clock = std::chrono::high_resolution_clock;

static double elapsed_ms(Clock::time_point start) {
    auto now = Clock::now();
    return std::chrono::duration<double, std::milli>(now - start).count();
}

// ──────────────────────────────────────────────────────────
//  Đường ống hoàn chỉnh: Đọc → Lọc → Tái tạo → Rút gọn →
//                       Trích xuất AR → Xuất GLB
// ──────────────────────────────────────────────────────────

PipelineResult OptimizationPipeline::run(const PipelineOptions& opts) {
    PipelineResult result;
    PipelineStats& st = result.stats;
    auto t_total = Clock::now();

    // ── Giai đoạn 1: Đọc PLY ──────────────────────────────
    std::cout << "\n[PIPELINE] ──── Stage 1/6: Đọc dữ liệu PLY ────" << std::endl;
    auto t0 = Clock::now();
    PlyReader reader;
    auto splats = reader.read(opts.input_path);
    st.time_read_ms  = elapsed_ms(t0);
    st.input_splats   = splats.size();

    // ── Giai đoạn 2: Lọc nhiễu ────────────────────────────
    std::cout << "\n[PIPELINE] ──── Stage 2/6: Lọc nhiễu hạt Gaussian ────" << std::endl;
    t0 = Clock::now();
    GsplatFilter filter;
    auto filtered = filter.run(splats, opts.filter);
    st.time_filter_ms  = elapsed_ms(t0);
    st.filtered_splats = filtered.size();

    // Giải phóng bộ nhớ của hạt thô sớm
    splats.clear();
    splats.shrink_to_fit();

    // ── Giai đoạn 3: Tái tạo bề mặt Poisson ───────────────
    std::cout << "\n[PIPELINE] ──── Stage 3/6: Tái tạo lưới bề mặt ────" << std::endl;
    t0 = Clock::now();
    MeshGenerator gen;
    auto mesh = gen.run(filtered, opts.meshing);
    st.time_mesh_ms    = elapsed_ms(t0);
    st.mesh_vertices   = mesh.vertices.size() / 3;
    st.mesh_triangles  = mesh.indices.size() / 3;

    filtered.clear();
    filtered.shrink_to_fit();

    // ── Giai đoạn 4: Rút gọn lưới ─────────────────────────
    std::cout << "\n[PIPELINE] ──── Stage 4/6: Tối ưu & Rút gọn lưới ────" << std::endl;
    t0 = Clock::now();
    MeshOptimizer optimizer;
    mesh = optimizer.run(mesh, opts.optimize);
    st.time_optimize_ms    = elapsed_ms(t0);
    st.optimized_triangles = mesh.indices.size() / 3;

    // ── Giai đoạn 5: Phân loại nhãn AR ────────────────────
    std::cout << "\n[PIPELINE] ──── Stage 5/6: Phân loại nhãn AR cho mặt ────" << std::endl;
    t0 = Clock::now();
    ArExtractor extractor;
    auto labels = extractor.run(mesh, opts.ar);
    st.time_extract_ms  = elapsed_ms(t0);
    st.occlusion_faces  = labels.occlusion_faces.size();
    st.nav_faces        = labels.nav_faces.size();

    // ── Giai đoạn 6: Xuất file GLB ───────────────────────
    std::cout << "\n[PIPELINE] ──── Stage 6/6: Xuất file GLB thành phẩm ────" << std::endl;
    t0 = Clock::now();
    GlbExporter exporter;
    exporter.write(mesh, labels, opts.output_path);
    st.time_export_ms = elapsed_ms(t0);

    st.time_total_ms = elapsed_ms(t_total);

    result.mesh   = std::move(mesh);
    result.labels = std::move(labels);

    // In báo cáo chi tiết hiệu suất qua stats_printer
    stats_printer::print(st);
    return result;
}

// ──────────────────────────────────────────────────────────
//  Phiên bản tương thích ngược với main.cpp cũ
// ──────────────────────────────────────────────────────────

void OptimizationPipeline::run(const std::string& input_path,
                               const std::string& output_path,
                               float opacity_thresh,
                               int target_faces) {
    PipelineOptions opts;
    opts.input_path  = input_path;
    opts.output_path = output_path;
    opts.filter.opacity_threshold        = opacity_thresh;
    opts.optimize.target_triangles       = static_cast<size_t>(target_faces);
    run(opts);
}