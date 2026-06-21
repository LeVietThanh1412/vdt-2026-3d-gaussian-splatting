#include "processor/gsplat_filter.hpp"

#include <open3d/Open3D.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

// ──────────────────────────────────────────────────────────
//  Giai đoạn 1: Lọc bỏ hạt kém chất lượng theo tiêu chí,
//  sau đó tùy chọn loại bỏ điểm ngoại lai thống kê.
// ──────────────────────────────────────────────────────────

// Kiểm tra hạt có chứa giá trị NaN hay không
static bool is_nan_splat(const GaussianSplat& s) {
    return std::isnan(s.x) || std::isnan(s.y) || std::isnan(s.z) ||
           std::isnan(s.opacity) ||
           std::isnan(s.scale[0]) || std::isnan(s.scale[1]) || std::isnan(s.scale[2]);
}

std::vector<GaussianSplat> GsplatFilter::run(
        const std::vector<GaussianSplat>& input,
        const FilterOptions& opts) {

    std::cout << "[FILTER] Đầu vào: " << input.size() << " hạt." << std::endl;

    // ── Lượt 1: Lọc chất lượng cơ bản ──────────────────
    std::vector<GaussianSplat> good;
    good.reserve(input.size());

    size_t nan_count = 0, opacity_count = 0, scale_count = 0;

    for (const auto& s : input) {
        if (is_nan_splat(s))                          { ++nan_count;     continue; }
        if (s.opacity < opts.opacity_threshold)       { ++opacity_count; continue; }
        if (s.scale[0] > opts.max_scale ||
            s.scale[1] > opts.max_scale ||
            s.scale[2] > opts.max_scale)              { ++scale_count;   continue; }
        good.push_back(s);
    }

    std::cout << "[FILTER] Đã loại bỏ – NaN: " << nan_count
              << "  opacity<" << opts.opacity_threshold << ": " << opacity_count
              << "  scale>" << opts.max_scale << ": " << scale_count << std::endl;
    std::cout << "[FILTER] Sau lọc cơ bản: " << good.size() << " hạt." << std::endl;

    // ── Lượt 2: Loại bỏ điểm ngoại lai thống kê (SOR) qua Open3D ───
    if (opts.enable_sor && good.size() > static_cast<size_t>(opts.knn_neighbours)) {
        // Xây dựng đám mây điểm Open3D từ vị trí các hạt còn lại
        auto cloud = std::make_shared<open3d::geometry::PointCloud>();
        cloud->points_.resize(good.size());
        for (size_t i = 0; i < good.size(); ++i) {
            cloud->points_[i] = Eigen::Vector3d(good[i].x, good[i].y, good[i].z);
        }

        // Chạy thuật toán SOR
        std::vector<size_t> inlier_idx;
        std::shared_ptr<open3d::geometry::PointCloud> filtered_cloud;
        std::tie(filtered_cloud, inlier_idx) =
            cloud->RemoveStatisticalOutliers(opts.knn_neighbours, opts.std_ratio);

        // Chỉ giữ lại các điểm nằm trong phạm vi
        std::vector<GaussianSplat> after_sor;
        after_sor.reserve(inlier_idx.size());
        for (size_t idx : inlier_idx) {
            after_sor.push_back(good[idx]);
        }
        good.swap(after_sor);

        std::cout << "[FILTER] Sau SOR (k=" << opts.knn_neighbours
                  << ", std=" << opts.std_ratio << "): "
                  << good.size() << " hạt." << std::endl;
    }

    return good;
}
