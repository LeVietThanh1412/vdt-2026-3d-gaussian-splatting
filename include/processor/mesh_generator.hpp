#pragma once

#include "core/types.hpp"
#include <vector>

/// Chuyển đổi các hạt Gaussian đã lọc thành lưới tam giác
/// thông qua thuật toán tái tạo bề mặt Poisson của Open3D.
class MeshGenerator {
public:
    /// Xây dựng lưới tam giác từ vị trí và màu sắc các hạt.
    MeshData run(const std::vector<GaussianSplat>& splats,
                 const MeshingOptions& opts);
};
