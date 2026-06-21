#pragma once

#include "core/types.hpp"
#include <vector>

/// Lọc bỏ các hạt Gaussian chất lượng thấp hoặc bị nhiễu.
///  - Loại bỏ hạt có độ mờ dưới ngưỡng
///  - Loại bỏ hạt có tỷ lệ bất thường hoặc giá trị NaN
///  - Tùy chọn: loại bỏ điểm ngoại lai thống kê (kNN)
class GsplatFilter {
public:
    /// Chạy bộ lọc và trả về danh sách hạt vượt qua toàn bộ tiêu chí.
    std::vector<GaussianSplat> run(const std::vector<GaussianSplat>& input,
                                   const FilterOptions& opts);
};
