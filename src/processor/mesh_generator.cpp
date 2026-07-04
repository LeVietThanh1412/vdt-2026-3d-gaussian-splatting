#include "processor/mesh_generator.hpp"
#include "core/mesh_converter.hpp"

#include <open3d/Open3D.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <tuple>

// ──────────────────────────────────────────────────────────
//  Giai đoạn 2: Chuyển đổi đám mây hạt → Đám mây điểm
//  Open3D → Tái tạo bề mặt Poisson → MeshData
// ──────────────────────────────────────────────────────────

MeshData MeshGenerator::run(const std::vector<GaussianSplat>& splats,
                            const MeshingOptions& opts) {

    std::cout << "[MESH_GEN] Đang xây dựng đám mây điểm từ " << splats.size()
              << " hạt ..." << std::endl;

    // ── 1. Xây dựng đám mây điểm Open3D ─────────────────
    auto cloud = std::make_shared<open3d::geometry::PointCloud>();
    cloud->points_.resize(splats.size());
    cloud->colors_.resize(splats.size());

    for (size_t i = 0; i < splats.size(); ++i) {
        cloud->points_[i] = Eigen::Vector3d(splats[i].x, splats[i].y, splats[i].z);
        cloud->colors_[i] = Eigen::Vector3d(splats[i].color[0],
                                             splats[i].color[1],
                                             splats[i].color[2]);
    }

    // ── 2. Ước lượng pháp tuyến (bắt buộc cho Poisson) ──
    std::cout << "[MESH_GEN] Đang ước lượng pháp tuyến ..." << std::endl;
    cloud->EstimateNormals(
        open3d::geometry::KDTreeSearchParamHybrid(/*bán_kính=*/0.1, /*số_láng_giềng_tối_đa=*/30));
    cloud->OrientNormalsConsistentTangentPlane(/*k=*/15);

    // Kiểm tra hướng pháp tuyến trung bình theo trục Y. Nếu hướng xuống (âm), lật ngược lại toàn bộ.
    double avg_y = 0.0;
    for (const auto& normal : cloud->normals_) {
        avg_y += normal.y();
    }
    if (!cloud->normals_.empty()) {
        avg_y /= static_cast<double>(cloud->normals_.size());
    }
    if (avg_y < 0.0) {
        std::cout << "[MESH_GEN] Phát hiện pháp tuyến hướng xuống dưới (avg_y = " << avg_y 
                  << "). Đang lật ngược lại hướng lên trên." << std::endl;
        for (auto& normal : cloud->normals_) {
            normal = -normal;
        }
    }

    // ── 3. Tái tạo bề mặt Poisson ───────────────────────
    std::cout << "[MESH_GEN] Đang chạy tái tạo Poisson (độ sâu="
              << opts.depth << ") ..." << std::endl;

    std::shared_ptr<open3d::geometry::TriangleMesh> mesh;
    std::vector<double> densities;
    // Trong mesh_generator.cpp, tìm dòng này:
    std::tie(mesh, densities) = open3d::geometry::TriangleMesh::CreateFromPointCloudPoisson(
        *cloud, opts.depth, 0, 0.8f, opts.linear_fit);

    // ── 4. Cắt tỉa đỉnh có mật độ thấp ─────────────────
    if (!densities.empty()) {
        // 1. Tính ngưỡng mật độ dựa trên phần trăm (quantile)
        std::vector<double> sorted_d = densities;
        std::sort(sorted_d.begin(), sorted_d.end());
        
        // Tăng density_quantile lên 0.05 hoặc 0.1 nếu ảnh vẫn xấu
        size_t cutoff_idx = static_cast<size_t>(opts.density_quantile * static_cast<double>(sorted_d.size()));
        double density_threshold = sorted_d[cutoff_idx];

        // 2. Loại bỏ các đỉnh có mật độ dưới ngưỡng
        std::vector<size_t> remove_idx;
        for (size_t i = 0; i < densities.size(); ++i) {
            // Chỉ giữ lại những vùng có mật độ cao hơn ngưỡng
            if (densities[i] < density_threshold) {
                remove_idx.push_back(i);
            }
        }
        
        // 3. Thực hiện xóa đỉnh (Hàm này của Open3D sẽ tự xóa các tam giác liên quan)
        mesh->RemoveVerticesByIndex(remove_idx);
    }

    // ── 5. Chuyển màu sắc qua tìm kiếm láng giềng gần nhất ─
    if (!mesh->vertices_.empty()) {
        auto tree = std::make_shared<open3d::geometry::KDTreeFlann>(*cloud);
        mesh->vertex_colors_.resize(mesh->vertices_.size());
        for (size_t i = 0; i < mesh->vertices_.size(); ++i) {
            std::vector<int> idx(1);
            std::vector<double> dist(1);
            tree->SearchKNN(mesh->vertices_[i], 1, idx, dist);
            mesh->vertex_colors_[i] = cloud->colors_[idx[0]];
        }
    }

    // --- Sếp thêm vào đây ---
    // OrientTriangles giúp lật các mặt tam giác về cùng một hướng
    // để tránh hiện tượng "tàng hình" khi nhìn mặt sau
    mesh->OrientTriangles();

    // ComputeVertexNormals nên gọi lại sau khi lật mặt
    mesh->ComputeVertexNormals();

    // ── 6. Chuyển đổi sang MeshData bằng tiện ích dùng chung ──
    MeshData out = mesh_converter::from_open3d(*mesh);

    const size_t nv = mesh->vertices_.size();
    const size_t nf = mesh->triangles_.size();
    std::cout << "[MESH_GEN] Kết quả: " << nv << " đỉnh, " << nf
              << " tam giác." << std::endl;
    return out;
}
