#pragma once

#include <cstdint>
#include <string>
#include <vector>

// ──────────────────────────────────────────────────────────
//  Các kiểu dữ liệu cốt lõi cho đường ống xử lý
//  3DGS → Mesh → AR
// ──────────────────────────────────────────────────────────

/// Một hạt Gaussian đơn lẻ được phân tích từ file .ply.
struct GaussianSplat {
    float x, y, z;       // Vị trí trong không gian 3D
    float opacity;        // Độ trong suốt (alpha)
    float scale[3];       // Tỷ lệ bất đẳng hướng (sx, sy, sz)
    float rot[4];         // Quaternion xoay (qw, qx, qy, qz)
    float color[3];       // Màu sắc RGB từ hệ số SH bậc 0 [0-1]
};

/// Lưới tam giác được tạo ra từ quá trình tái tạo bề mặt hoặc rút gọn.
struct MeshData {
    std::vector<float>    vertices;   // Tọa độ đỉnh dạng phẳng (v0x,v0y,v0z, v1x,...)
    std::vector<float>    normals;    // Vector pháp tuyến tại mỗi đỉnh
    std::vector<float>    colors;     // Màu RGB tại mỗi đỉnh [0-1]
    std::vector<uint32_t> indices;    // Chỉ số tam giác (3 chỉ số mỗi mặt)
};

/// Nhãn phân loại các mặt tam giác phục vụ AR.
struct MeshLabels {
    std::vector<uint32_t> occlusion_faces;  // Chỉ số mặt → lưới che khuất
    std::vector<uint32_t> nav_faces;        // Chỉ số mặt → lưới điều hướng
};

// ──────────── Cấu hình cho từng giai đoạn xử lý ────────────

/// Tùy chọn cho bước lọc nhiễu hạt Gaussian.
struct FilterOptions {
    float opacity_threshold = 0.1f;   // Loại bỏ hạt có độ mờ dưới ngưỡng này
    float max_scale         = 10.0f;  // Loại bỏ hạt có trục bất kỳ vượt quá giá trị này
    int   knn_neighbours    = 30;     // Số láng giềng cho thuật toán loại bỏ điểm ngoại lai
    float std_ratio         = 2.0f;   // Hệ số nhân độ lệch chuẩn cho SOR
    bool  enable_sor        = true;   // Bật/tắt loại bỏ điểm ngoại lai thống kê
};

/// Tùy chọn cho bước tái tạo bề mặt Poisson.
struct MeshingOptions {
    int   depth       = 9;     // Độ sâu cây bát phân (octree) cho Poisson
    float scale       = 1.1f;  // Hệ số đệm kích thước khối bao
    bool  linear_fit  = false; // Bật nội suy tuyến tính
    float density_quantile = 0.01f; // Cắt tỉa đỉnh có mật độ thấp (phần trăm)
};

/// Tùy chọn cho bước rút gọn lưới tam giác.
struct OptimizeOptions {
    size_t target_triangles = 50000;  // Số tam giác mục tiêu sau rút gọn
    bool   recompute_normals = true;  // Tính lại pháp tuyến sau khi rút gọn
};

/// Tùy chọn cho bước trích xuất dữ liệu AR.
struct ArOptions {
    float max_slope_deg = 45.0f;  // Mặt có góc nghiêng ≤ giá trị này so với trục Y → lưới điều hướng
};

/// Tập hợp toàn bộ tùy chọn truyền vào đường ống xử lý.
struct PipelineOptions {
    std::string   input_path;   // Đường dẫn file .ply đầu vào
    std::string   output_path;  // Đường dẫn file .glb xuất ra
    FilterOptions filter;       // Cấu hình lọc nhiễu
    MeshingOptions meshing;     // Cấu hình tái tạo bề mặt
    OptimizeOptions optimize;   // Cấu hình rút gọn lưới
    ArOptions     ar;           // Cấu hình trích xuất AR
};

/// Thống kê thu thập được trong quá trình chạy đường ống.
struct PipelineStats {
    size_t input_splats      = 0;  // Số hạt đầu vào
    size_t filtered_splats   = 0;  // Số hạt sau lọc
    size_t mesh_vertices     = 0;  // Số đỉnh lưới thô
    size_t mesh_triangles    = 0;  // Số tam giác lưới thô
    size_t optimized_triangles = 0; // Số tam giác sau rút gọn
    size_t occlusion_faces   = 0;  // Số mặt che khuất
    size_t nav_faces         = 0;  // Số mặt điều hướng
    double time_read_ms      = 0;  // Thời gian đọc file (ms)
    double time_filter_ms    = 0;  // Thời gian lọc nhiễu (ms)
    double time_mesh_ms      = 0;  // Thời gian tái tạo bề mặt (ms)
    double time_optimize_ms  = 0;  // Thời gian rút gọn lưới (ms)
    double time_extract_ms   = 0;  // Thời gian trích xuất AR (ms)
    double time_export_ms    = 0;  // Thời gian xuất file GLB (ms)
    double time_total_ms     = 0;  // Tổng thời gian xử lý (ms)
};
