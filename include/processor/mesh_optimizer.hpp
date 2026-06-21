#pragma once

#include "core/types.hpp"

/// Đơn giản hóa lưới tam giác bằng thuật toán rút gọn sai số bậc hai
/// (Quadric Error Decimation) về số tam giác mục tiêu.
class MeshOptimizer {
public:
    /// Rút gọn lưới đầu vào và tùy chọn tính lại pháp tuyến.
    MeshData run(const MeshData& input, const OptimizeOptions& opts);
};
