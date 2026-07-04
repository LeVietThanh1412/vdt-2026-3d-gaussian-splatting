#include "processor/mesh_optimizer.hpp"
#include "core/mesh_converter.hpp"

#include <open3d/Open3D.h>

#include <iostream>
#include <limits>

// ──────────────────────────────────────────────────────────
//  Giai đoạn 3: Rút gọn lưới tam giác bằng thuật toán
//  sai số bậc hai (Quadric Error Decimation) về số tam
//  giác mục tiêu, tùy chọn tính lại pháp tuyến.
// ──────────────────────────────────────────────────────────

MeshData MeshOptimizer::run(const MeshData& input, const OptimizeOptions& opts) {

    const size_t nv_in = input.vertices.size() / 3;
    const size_t nf_in = input.indices.size() / 3;
    std::cout << "[OPTIMIZE] Lưới đầu vào: " << nv_in << " đỉnh, "
              << nf_in << " tam giác." << std::endl;

    // Nếu đã dưới mục tiêu thì bỏ qua
    if (nf_in <= opts.target_triangles) {
        std::cout << "[OPTIMIZE] Đã ở dưới mục tiêu (" << opts.target_triangles
                  << "). Bỏ qua rút gọn." << std::endl;
        return input;
    }

    // ── Chuyển đổi sang Open3D bằng tiện ích dùng chung ─
    auto mesh = mesh_converter::to_open3d(input);

    // ── Rút gọn bằng sai số bậc hai ─────────────────────
    std::cout << "[OPTIMIZE] Đang rút gọn về " << opts.target_triangles
              << " tam giác ..." << std::endl;

    auto simplified = mesh->SimplifyQuadricDecimation(
        static_cast<int>(opts.target_triangles),
        std::numeric_limits<double>::infinity(),
        1.0);

    // Dọn dẹp cơ bản
    simplified->RemoveDegenerateTriangles();
    simplified->RemoveDuplicatedVertices();
    simplified->RemoveDuplicatedTriangles();

    // ── Xóa các mảnh lưới nhỏ (Noise Pruning) ──
    auto [cluster_ids, cluster_sizes, cluster_areas] = simplified->ClusterConnectedTriangles();
    std::vector<size_t> triangles_to_remove;
    for (size_t i = 0; i < simplified->triangles_.size(); ++i) {
        if (cluster_sizes[cluster_ids[i]] < 100) {
            triangles_to_remove.push_back(i);
        }
    }
    simplified->RemoveTrianglesByIndex(triangles_to_remove);
    simplified->RemoveUnreferencedVertices();
    simplified = simplified->FilterSmoothLaplacian(2, 0.5);
    simplified->OrientTriangles();

    if (opts.recompute_normals) {
        simplified->ComputeVertexNormals();
    }

    // ── Chuyển đổi ngược bằng tiện ích dùng chung ───────
    MeshData out = mesh_converter::from_open3d(*simplified);

    const size_t nv = simplified->vertices_.size();
    const size_t nf = simplified->triangles_.size();
    std::cout << "[OPTIMIZE] Kết quả: " << nv << " đỉnh, " << nf
              << " tam giác." << std::endl;
    return out;
}