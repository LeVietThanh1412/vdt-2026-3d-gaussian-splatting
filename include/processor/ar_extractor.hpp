#pragma once

#include "core/types.hpp"

/// Phân loại các mặt tam giác của lưới thành hai nhóm:
/// lưới che khuất (occlusion) và lưới điều hướng (navigation)
/// dựa trên hướng vector pháp tuyến bề mặt.
class ArExtractor {
public:
    /// Phân tích pháp tuyến các mặt:
    ///  - Mặt có pháp tuyến nằm trong góc max_slope_deg so với +Y → lưới điều hướng
    ///  - Tất cả mặt còn lại → lưới che khuất
    MeshLabels run(const MeshData& mesh, const ArOptions& opts);
};
