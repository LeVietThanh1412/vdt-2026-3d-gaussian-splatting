#pragma once

#include "core/types.hpp"
#include <string>

/// Kết quả trả về sau khi chạy toàn bộ đường ống xử lý.
struct PipelineResult {
    MeshData     mesh;    // Lưới tam giác đã tối ưu
    MeshLabels   labels;  // Nhãn phân loại AR
    PipelineStats stats;  // Thống kê hiệu suất
};

/// Điều phối toàn bộ đường ống xử lý:
///   Đọc PLY → Lọc nhiễu → Tái tạo bề mặt → Rút gọn lưới → Trích xuất AR → Xuất GLB
class OptimizationPipeline {
public:
    /// Chạy tất cả các giai đoạn theo thứ tự và ghi file .glb đầu ra.
    PipelineResult run(const PipelineOptions& opts);

    /// Phiên bản rút gọn tương thích ngược với main.cpp cũ.
    void run(const std::string& input_path,
             const std::string& output_path,
             float opacity_thresh,
             int   target_faces);
};
