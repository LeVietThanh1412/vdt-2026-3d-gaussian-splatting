#include "io/ply_reader.hpp"
#include "core/constants.hpp"
#include "happly.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

// ──────────────────────────────────────────────────────────
//  Hàm trợ giúp: đọc thuộc tính kiểu float, trả về vector
//  giá trị mặc định nếu thuộc tính không tồn tại.
// ──────────────────────────────────────────────────────────
static std::vector<double> try_property(happly::Element& elem,
                                         const char* name,
                                         size_t count,
                                         double fallback) {
    try {
        return elem.getProperty<double>(name);
    } catch (...) {
        std::cout << "[PLY_READER] Thuộc tính '" << name
                  << "' không tìm thấy – dùng mặc định " << fallback << std::endl;
        return std::vector<double>(count, fallback);
    }
}

// ──────────────────────────────────────────────────────────
//  Hàm trợ giúp: đọc thuộc tính linh hoạt với nhiều kiểu
//  dữ liệu khác nhau để tránh lỗi ép kiểu
//  (ví dụ: red/green/blue kiểu uchar).
// ──────────────────────────────────────────────────────────
static std::vector<double> get_property_dynamic(happly::Element& elem, const std::string& name) {
    // Thử đọc lần lượt: double → float → unsigned char → int
    try { return elem.getProperty<double>(name); } catch (...) {}
    try {
        auto raw = elem.getProperty<float>(name);
        return std::vector<double>(raw.begin(), raw.end());
    } catch (...) {}
    try {
        auto raw = elem.getProperty<unsigned char>(name);
        return std::vector<double>(raw.begin(), raw.end());
    } catch (...) {}
    try {
        auto raw = elem.getProperty<int>(name);
        return std::vector<double>(raw.begin(), raw.end());
    } catch (...) {}
    return {};
}

// Chuyển đổi hệ số SH bậc 0 (DC) → giá trị RGB tuyến tính (giới hạn [0,1])
static float sh_dc_to_rgb(double dc) {
    constexpr double C0 = 0.28209479177387814;  // Hằng số chuẩn hóa SH bậc 0
    double v = 0.5 + C0 * dc; 
    if (v < 0.0) v = 0.0;
    if (v > 1.0) v = 1.0;
    return static_cast<float>(v);
}

// Hàm kích hoạt sigmoid cho giá trị opacity dạng logit
static float sigmoid(double x) {
    return static_cast<float>(1.0 / (1.0 + std::exp(-x)));
}

std::vector<GaussianSplat> PlyReader::read(const std::string& file_path) {
    namespace c = constants::ply;

    std::cout << "[PLY_READER] Đang tải " << file_path << " ..." << std::endl;

    happly::PLYData ply(file_path);
    auto& vtx = ply.getElement(c::ELEM_VERTEX);
    const size_t n = vtx.getProperty<double>(c::PROP_X).size();

    std::cout << "[PLY_READER] Tìm thấy " << n << " đỉnh." << std::endl;

    // Bắt buộc: tọa độ vị trí
    auto px = vtx.getProperty<double>(c::PROP_X);
    auto py = vtx.getProperty<double>(c::PROP_Y);
    auto pz = vtx.getProperty<double>(c::PROP_Z);

    // Phát hiện động các loại thuộc tính
    bool has_opacity = vtx.hasProperty(c::PROP_OPACITY) || vtx.hasProperty("opacity");
    bool has_scale   = vtx.hasProperty(c::PROP_SCALE_0) || vtx.hasProperty("scale_0");
    bool has_rot     = vtx.hasProperty(c::PROP_ROT_0) || vtx.hasProperty("rot_0");
    bool has_sh      = vtx.hasProperty(c::PROP_F_DC_0) || vtx.hasProperty("f_dc_0") || vtx.hasProperty("features_0");

    // Tìm kiếm tên thuộc tính màu RGB với nhiều biến thể
    std::vector<std::string> red_candidates = {"red", "diffuse_red", "r"};
    std::vector<std::string> green_candidates = {"green", "diffuse_green", "g"};
    std::vector<std::string> blue_candidates = {"blue", "diffuse_blue", "b"};

    std::string red_prop = "", green_prop = "", blue_prop = "";
    for (const auto& name : red_candidates) {
        if (vtx.hasProperty(name)) { red_prop = name; break; }
    }
    for (const auto& name : green_candidates) {
        if (vtx.hasProperty(name)) { green_prop = name; break; }
    }
    for (const auto& name : blue_candidates) {
        if (vtx.hasProperty(name)) { blue_prop = name; break; }
    }
    bool has_rgb = !red_prop.empty() && !green_prop.empty() && !blue_prop.empty();

    // Đọc các thuộc tính tùy chọn bằng hàm đọc linh hoạt
    std::vector<double> op, s0, s1, s2, r0, r1, r2, r3, dc0, dc1, dc2;
    std::vector<double> r_val, g_val, b_val;

    if (has_opacity) {
        op = get_property_dynamic(vtx, vtx.hasProperty(c::PROP_OPACITY) ? c::PROP_OPACITY : "opacity");
    }
    if (has_scale) {
        s0 = get_property_dynamic(vtx, vtx.hasProperty(c::PROP_SCALE_0) ? c::PROP_SCALE_0 : "scale_0");
        s1 = get_property_dynamic(vtx, vtx.hasProperty(c::PROP_SCALE_1) ? c::PROP_SCALE_1 : "scale_1");
        s2 = get_property_dynamic(vtx, vtx.hasProperty(c::PROP_SCALE_2) ? c::PROP_SCALE_2 : "scale_2");
    }
    if (has_rot) {
        r0 = get_property_dynamic(vtx, vtx.hasProperty(c::PROP_ROT_0) ? c::PROP_ROT_0 : "rot_0");
        r1 = get_property_dynamic(vtx, vtx.hasProperty(c::PROP_ROT_1) ? c::PROP_ROT_1 : "rot_1");
        r2 = get_property_dynamic(vtx, vtx.hasProperty(c::PROP_ROT_2) ? c::PROP_ROT_2 : "rot_2");
        r3 = get_property_dynamic(vtx, vtx.hasProperty(c::PROP_ROT_3) ? c::PROP_ROT_3 : "rot_3");
    }
    if (has_sh) {
        // Đọc hệ số Spherical Harmonics bậc 0 (DC) cho 3 kênh màu
        std::string dc0_name = vtx.hasProperty(c::PROP_F_DC_0) ? c::PROP_F_DC_0 : (vtx.hasProperty("f_dc_0") ? "f_dc_0" : "features_0");
        std::string dc1_name = vtx.hasProperty(c::PROP_F_DC_1) ? c::PROP_F_DC_1 : (vtx.hasProperty("f_dc_1") ? "f_dc_1" : "features_1");
        std::string dc2_name = vtx.hasProperty(c::PROP_F_DC_2) ? c::PROP_F_DC_2 : (vtx.hasProperty("f_dc_2") ? "f_dc_2" : "features_2");
        dc0 = get_property_dynamic(vtx, dc0_name);
        dc1 = get_property_dynamic(vtx, dc1_name);
        dc2 = get_property_dynamic(vtx, dc2_name);
    } else if (has_rgb) {
        // Không có dữ liệu 3DGS → dùng thuộc tính RGB thường
        std::cout << "[PLY_READER] Không tìm thấy dữ liệu 3DGS. Dùng thuộc tính RGB: "
                  << red_prop << ", " << green_prop << ", " << blue_prop << std::endl;
        r_val = get_property_dynamic(vtx, red_prop);
        g_val = get_property_dynamic(vtx, green_prop);
        b_val = get_property_dynamic(vtx, blue_prop);
    }

    // Kiểm tra xem giá trị RGB có nằm trong dải [0-255] hay [0-1]
    bool rgb_needs_scale = false;
    if (has_rgb && !has_sh) {
        for (size_t i = 0; i < n; ++i) {
            if (r_val[i] > 1.0 || g_val[i] > 1.0 || b_val[i] > 1.0) {
                rgb_needs_scale = true;  // Cần chia cho 255 để chuẩn hóa
                break;
            }
        }
    }

    // Xây dựng danh sách hạt Gaussian
    std::vector<GaussianSplat> splats(n);
    for (size_t i = 0; i < n; ++i) {
        auto& s  = splats[i];
        s.x      = static_cast<float>(px[i]);
        s.y      = static_cast<float>(py[i]);
        s.z      = static_cast<float>(pz[i]);

        // Độ trong suốt: áp dụng hàm sigmoid lên giá trị logit
        if (has_opacity) {
            s.opacity = sigmoid(op[i]);
        } else {
            s.opacity = 1.0f;  // Mặc định: hoàn toàn đục
        }

        // Tỷ lệ: áp dụng hàm mũ lên giá trị log
        if (has_scale) {
            s.scale[0] = static_cast<float>(std::exp(s0[i]));
            s.scale[1] = static_cast<float>(std::exp(s1[i]));
            s.scale[2] = static_cast<float>(std::exp(s2[i]));
        } else {
            s.scale[0] = 0.01f;  // Mặc định cho Point Cloud thường
            s.scale[1] = 0.01f;
            s.scale[2] = 0.01f;
        }

        // Quaternion xoay
        if (has_rot) {
            s.rot[0] = static_cast<float>(r0[i]);
            s.rot[1] = static_cast<float>(r1[i]);
            s.rot[2] = static_cast<float>(r2[i]);
            s.rot[3] = static_cast<float>(r3[i]);
        } else {
            s.rot[0] = 1.0f;  // Mặc định: không xoay (quaternion đơn vị)
            s.rot[1] = 0.0f;
            s.rot[2] = 0.0f;
            s.rot[3] = 0.0f;
        }

        // Màu sắc
        if (has_sh) {
            // Chuyển đổi hệ số SH bậc 0 → RGB
            s.color[0] = sh_dc_to_rgb(dc0[i]);
            s.color[1] = sh_dc_to_rgb(dc1[i]);
            s.color[2] = sh_dc_to_rgb(dc2[i]);
        } else if (has_rgb) {
            // Dùng giá trị RGB trực tiếp, chuẩn hóa nếu cần
            float factor = rgb_needs_scale ? 255.0f : 1.0f;
            s.color[0] = static_cast<float>(r_val[i]) / factor;
            s.color[1] = static_cast<float>(g_val[i]) / factor;
            s.color[2] = static_cast<float>(b_val[i]) / factor;
        } else {
            // Không có thông tin màu → gán màu xám mặc định
            s.color[0] = 0.8f;
            s.color[1] = 0.8f;
            s.color[2] = 0.8f;
        }
    }

    std::cout << "[PLY_READER] Trích xuất thành công " << splats.size()
              << " hạt Gaussian." << std::endl;
    return splats;
}