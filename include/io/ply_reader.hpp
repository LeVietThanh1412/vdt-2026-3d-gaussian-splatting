#pragma once

#include "core/types.hpp"
#include <string>
#include <vector>

/// Đọc file .ply chứa dữ liệu 3D Gaussian Splatting và trả về danh sách hạt.
/// Hỗ trợ cả file 3DGS chuẩn (f_dc, opacity, scale, rot) lẫn Point Cloud
/// thường (red, green, blue).
class PlyReader {
public:
    /// Phân tích file PLY và trích xuất toàn bộ thuộc tính hạt Gaussian.
    /// Tự động gán giá trị mặc định cho các thuộc tính tùy chọn bị thiếu.
    std::vector<GaussianSplat> read(const std::string& file_path);
};