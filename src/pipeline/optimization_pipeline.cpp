#include "pipeline/optimization_pipeline.hpp"
#include "core/stats_printer.hpp"
#include "io/ply_reader.hpp"
#include "io/glb_exporter.hpp"
#include "processor/gsplat_filter.hpp"
#include "processor/mesh_generator.hpp"
#include "processor/mesh_optimizer.hpp"
#include "processor/ar_extractor.hpp"

#include <chrono>
#include <algorithm>
#include <cmath>    
#include <iostream>

// ──────────────────────────────────────────────────────────
//  Trình đo thời gian thực thi (ms)
// ──────────────────────────────────────────────────────────

using Clock = std::chrono::high_resolution_clock;

static double elapsed_ms(Clock::time_point start) {
    auto now = Clock::now();
    return std::chrono::duration<double, std::milli>(now - start).count();
}

static void normalize_mesh(MeshData& mesh) {
    if (mesh.vertices.empty()) return;

    // 1. Tìm giới hạn Bounding Box
    float min_x = mesh.vertices[0], max_x = mesh.vertices[0];
    float min_y = mesh.vertices[1], max_y = mesh.vertices[1];
    float min_z = mesh.vertices[2], max_z = mesh.vertices[2];

    const size_t num_verts = mesh.vertices.size() / 3;
    for (size_t i = 0; i < num_verts; ++i) {
        float x = mesh.vertices[i * 3 + 0];
        float y = mesh.vertices[i * 3 + 1];
        float z = mesh.vertices[i * 3 + 2];

        if (x < min_x) min_x = x; if (x > max_x) max_x = x;
        if (y < min_y) min_y = y; if (y > max_y) max_y = y;
        if (z < min_z) min_z = z; if (z > max_z) max_z = z;
    }

    // 2. Tính kích thước lớn nhất của các chiều
    float dx = max_x - min_x;
    float dy = max_y - min_y;
    float dz = max_z - min_z;
    float max_dim = std::max({dx, dy, dz});

    if (max_dim < 1e-5f) return;

    // 3. Scale về kích thước tối đa 1.0 mét, đưa tâm X Z về 0, đặt đáy Y tại 0
    float scale = 1.0f / max_dim;
    float cx = (min_x + max_x) / 2.0f;
    float cz = (min_z + max_z) / 2.0f;
    float cy_offset = min_y; // dịch chuyển đáy về Y = 0

    std::cout << "[PIPELINE] Chuẩn hóa Bounding Box:" << std::endl;
    std::cout << "  - Kích thước cũ: [" << dx << " x " << dy << " x " << dz << "]" << std::endl;
    std::cout << "  - Tỷ lệ scale: " << scale << std::endl;

    for (size_t i = 0; i < num_verts; ++i) {
        mesh.vertices[i * 3 + 0] = (mesh.vertices[i * 3 + 0] - cx) * scale;
        mesh.vertices[i * 3 + 1] = (mesh.vertices[i * 3 + 1] - cy_offset) * scale;
        mesh.vertices[i * 3 + 2] = (mesh.vertices[i * 3 + 2] - cz) * scale;
    }
}

// ──────────────────────────────────────────────────────────
//  Đường ống hoàn chỉnh: Đọc → Lọc → Tái tạo → Rút gọn →
//                       Trích xuất AR → Xuất GLB
// ──────────────────────────────────────────────────────────

// Logic tự suy luận dựa trên số lượng hạt (splats.size())
void auto_tune_options(PipelineOptions& opts, size_t num_splats) {
    // 1. Depth Poisson: Tăng độ sâu nếu file nhiều hạt (Dữ liệu lớn cần độ phân giải cao)
    // Công thức: Căn bậc 3 của số triệu điểm + 8 (đảm bảo depth từ 8-12)
    opts.meshing.depth = std::clamp(static_cast<int>(std::log2(num_splats) / 2), 8, 12);
    
    // 2. Target Triangles: Nếu file to mà ép xuống 50k mặt thì mất chi tiết -> tỉ lệ thuận với số hạt
    // Ví dụ: 1 triệu hạt -> 100k tam giác, 100k hạt -> 20k tam giác
    opts.optimize.target_triangles = std::clamp(static_cast<size_t>(num_splats / 10), 20000UL, 200000UL);
    
    // 3. Density Quantile: Tự điều chỉnh nếu splats quá nhiễu (Dữ liệu nhỏ thường nhiễu hơn)
    opts.meshing.density_quantile = (num_splats < 500000) ? 0.1f : 0.05f;

    std::cout << "[PIPELINE] Đã tự động điều chỉnh: Depth=" << opts.meshing.depth 
              << ", Target=" << opts.optimize.target_triangles << std::endl;
}

PipelineResult OptimizationPipeline::run(const PipelineOptions& opts) {
    PipelineResult result;
    PipelineStats& st = result.stats;
    auto t_total = Clock::now();

    // ── Giai đoạn 1: Đọc PLY ──────────────────────────────
    std::cout << "\n[PIPELINE] ──── Stage 1/6: Đọc dữ liệu PLY ────" << std::endl;
    auto t0 = Clock::now();
    PlyReader reader;
    auto splats = reader.read(opts.input_path);
    
    // Khởi tạo dynamic_opts và tự động điều chỉnh
    PipelineOptions dynamic_opts = opts;
    auto_tune_options(dynamic_opts, splats.size());

    st.time_read_ms  = elapsed_ms(t0);
    st.input_splats   = splats.size();

    // ── Giai đoạn 2: Lọc nhiễu (Phải khởi tạo filter tại đây) ──
    std::cout << "\n[PIPELINE] ──── Stage 2/6: Lọc nhiễu ────" << std::endl;
    t0 = Clock::now();
    GsplatFilter filter; 
    auto filtered = filter.run(splats, dynamic_opts.filter);
    st.time_filter_ms  = elapsed_ms(t0);
    st.filtered_splats = filtered.size();
    splats.clear(); splats.shrink_to_fit();

    // ── Giai đoạn 3: Tái tạo bề mặt (Khởi tạo generator) ──
    std::cout << "\n[PIPELINE] ──── Stage 3/6: Tái tạo lưới ────" << std::endl;
    t0 = Clock::now();
    MeshGenerator gen;
    auto mesh = gen.run(filtered, dynamic_opts.meshing);
    st.time_mesh_ms    = elapsed_ms(t0);
    // ... (các bước tiếp theo tương tự)

    // ── Giai đoạn 4: Rút gọn lưới ─────────────────────────
    std::cout << "\n[PIPELINE] ──── Stage 4/6: Tối ưu & Rút gọn lưới ────" << std::endl;
    t0 = Clock::now();
    MeshOptimizer optimizer;
    mesh = optimizer.run(mesh, opts.optimize);
    st.time_optimize_ms    = elapsed_ms(t0);
    st.optimized_triangles = mesh.indices.size() / 3;

    // Chuẩn hóa kích thước khung bao (Bounding Box Normalization)
    normalize_mesh(mesh);

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