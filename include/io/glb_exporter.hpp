#pragma once

#include "core/types.hpp"
#include <string>

/// Xuất cặp dữ liệu MeshData + MeshLabels thành file glTF nhị phân (.glb).
/// Tạo hai nút lưới riêng biệt: "OcclusionMesh" và "NavMesh".
class GlbExporter {
public:
    /// Ghi lưới tam giác ra đĩa dưới dạng file .glb.
    /// @param mesh      Lưới tam giác cần xuất.
    /// @param labels    Phân loại mặt (che khuất và điều hướng).
    /// @param file_path Đường dẫn đích (nên kết thúc bằng .glb).
    void write(const MeshData& mesh,
               const MeshLabels& labels,
               const std::string& file_path);
};
