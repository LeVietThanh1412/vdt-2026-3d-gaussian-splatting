#pragma once

namespace constants {

// Thông tin ứng dụng
constexpr const char* VERSION = "1.0.0";
constexpr const char* APP_NAME = "3DGS AR Structural Optimization Engine";

// Tham số mặc định cho đường ống xử lý
constexpr float  DEFAULT_OPACITY_THRESHOLD = 0.1f;   // Ngưỡng lọc độ mờ
constexpr float  DEFAULT_MAX_SCALE         = 10.0f;  // Giới hạn tỷ lệ tối đa
constexpr int    DEFAULT_POISSON_DEPTH     = 9;      // Độ sâu Poisson mặc định
constexpr size_t DEFAULT_TARGET_TRIANGLES  = 50000;  // Số tam giác mục tiêu
constexpr float  DEFAULT_NAV_SLOPE_DEG     = 45.0f;  // Góc nghiêng tối đa cho lưới điều hướng

// Tên thuộc tính PLY theo chuẩn 3D Gaussian Splatting
namespace ply {
constexpr const char* ELEM_VERTEX = "vertex";    // Phần tử đỉnh
constexpr const char* PROP_X      = "x";         // Tọa độ X
constexpr const char* PROP_Y      = "y";         // Tọa độ Y
constexpr const char* PROP_Z      = "z";         // Tọa độ Z
constexpr const char* PROP_OPACITY = "opacity";  // Độ trong suốt (dạng logit)
constexpr const char* PROP_SCALE_0 = "scale_0";  // Tỷ lệ trục 0 (dạng log)
constexpr const char* PROP_SCALE_1 = "scale_1";  // Tỷ lệ trục 1
constexpr const char* PROP_SCALE_2 = "scale_2";  // Tỷ lệ trục 2
constexpr const char* PROP_ROT_0   = "rot_0";    // Quaternion thành phần w
constexpr const char* PROP_ROT_1   = "rot_1";    // Quaternion thành phần x
constexpr const char* PROP_ROT_2   = "rot_2";    // Quaternion thành phần y
constexpr const char* PROP_ROT_3   = "rot_3";    // Quaternion thành phần z
constexpr const char* PROP_F_DC_0  = "f_dc_0";   // Hệ số Spherical Harmonics bậc 0 - kênh R
constexpr const char* PROP_F_DC_1  = "f_dc_1";   // Hệ số Spherical Harmonics bậc 0 - kênh G
constexpr const char* PROP_F_DC_2  = "f_dc_2";   // Hệ số Spherical Harmonics bậc 0 - kênh B
} // namespace ply

} // namespace constants
