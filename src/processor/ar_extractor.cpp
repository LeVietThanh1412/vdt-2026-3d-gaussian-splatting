#include "processor/ar_extractor.hpp"

#include <cmath>
#include <iostream>

// ──────────────────────────────────────────────────────────
//  Giai đoạn 5: Phân loại mỗi mặt tam giác thành lưới
//  điều hướng hoặc lưới che khuất dựa trên hướng pháp
//  tuyến bề mặt so với hướng LÊN (trục +Y).
// ──────────────────────────────────────────────────────────

MeshLabels ArExtractor::run(const MeshData& mesh, const ArOptions& opts) {
    MeshLabels labels;
    const size_t num_faces = mesh.indices.size() / 3;

    if (num_faces == 0) {
        std::cout << "[AR_EXTRACT] Không có mặt nào để phân loại." << std::endl;
        return labels;
    }

    // Tính ngưỡng cosine từ góc nghiêng tối đa
    const float cos_threshold = std::cos(opts.max_slope_deg * static_cast<float>(M_PI) / 180.0f);

    for (size_t f = 0; f < num_faces; ++f) {
        // Lấy chỉ số 3 đỉnh của tam giác
        uint32_t i0 = mesh.indices[f * 3 + 0];
        uint32_t i1 = mesh.indices[f * 3 + 1];
        uint32_t i2 = mesh.indices[f * 3 + 2];

        // Tọa độ các đỉnh
        float v0x = mesh.vertices[i0 * 3 + 0], v0y = mesh.vertices[i0 * 3 + 1], v0z = mesh.vertices[i0 * 3 + 2];
        float v1x = mesh.vertices[i1 * 3 + 0], v1y = mesh.vertices[i1 * 3 + 1], v1z = mesh.vertices[i1 * 3 + 2];
        float v2x = mesh.vertices[i2 * 3 + 0], v2y = mesh.vertices[i2 * 3 + 1], v2z = mesh.vertices[i2 * 3 + 2];

        // Tính vector cạnh
        float e1x = v1x - v0x, e1y = v1y - v0y, e1z = v1z - v0z;
        float e2x = v2x - v0x, e2y = v2y - v0y, e2z = v2z - v0z;

        // Tích chéo → pháp tuyến mặt
        float nx = e1y * e2z - e1z * e2y;
        float ny = e1z * e2x - e1x * e2z;
        float nz = e1x * e2y - e1y * e2x;

        float len = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (len < 1e-8f) {
            // Mặt suy biến → mặc định là che khuất
            labels.occlusion_faces.push_back(static_cast<uint32_t>(f));
            continue;
        }

        // Chuẩn hóa pháp tuyến
        nx /= len;  ny /= len;  nz /= len;

        // Tích vô hướng với hướng LÊN = (0, 1, 0)
        float dot_up = ny;  // Vì LÊN là (0,1,0) nên chỉ cần thành phần y

        if (dot_up >= cos_threshold) {
            labels.nav_faces.push_back(static_cast<uint32_t>(f));       // Lưới điều hướng
        } else {
            labels.occlusion_faces.push_back(static_cast<uint32_t>(f)); // Lưới che khuất
        }
    }

    std::cout << "[AR_EXTRACT] Mặt điều hướng: " << labels.nav_faces.size()
              << "  Mặt che khuất: " << labels.occlusion_faces.size()
              << "  (ngưỡng góc nghiêng: " << opts.max_slope_deg << "°)" << std::endl;

    return labels;
}
