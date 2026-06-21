#include "io/glb_exporter.hpp"

// tinygltf yêu cầu các định nghĩa này chính xác một lần trước khi include
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "tiny_gltf.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

// ──────────────────────────────────────────────────────────
//  Cấu trúc phụ trợ: chứa một phần lưới tam giác con
//  (trích xuất từ MeshData chính) để đưa vào glTF.
// ──────────────────────────────────────────────────────────

struct SubMesh {
    std::vector<float>    positions;  // Tọa độ đỉnh xyz dạng phẳng
    std::vector<float>    normals;    // Pháp tuyến xyz dạng phẳng
    std::vector<float>    colors;     // Màu sắc rgb dạng phẳng
    std::vector<uint32_t> indices;    // Chỉ số tam giác (đã được đánh số lại)
};

/// Trích xuất danh sách mặt tam giác trong `face_indices` từ lưới gốc,
/// đánh lại số đỉnh để lưới con độc lập hoàn toàn.
static SubMesh extract_submesh(const MeshData& mesh,
                               const std::vector<uint32_t>& face_indices) {
    SubMesh sub;
    if (face_indices.empty()) return sub;

    // Thu thập các đỉnh độc nhất được tham chiếu bởi các mặt này
    std::vector<uint32_t> old_verts;
    for (uint32_t fi : face_indices) {
        old_verts.push_back(mesh.indices[fi * 3 + 0]);
        old_verts.push_back(mesh.indices[fi * 3 + 1]);
        old_verts.push_back(mesh.indices[fi * 3 + 2]);
    }
    std::sort(old_verts.begin(), old_verts.end());
    old_verts.erase(std::unique(old_verts.begin(), old_verts.end()),
                    old_verts.end());

    // Xây dựng bản đồ chuyển đổi chỉ số đỉnh cũ → mới
    std::vector<uint32_t> remap(mesh.vertices.size() / 3, UINT32_MAX);
    for (uint32_t ni = 0; ni < static_cast<uint32_t>(old_verts.size()); ++ni) {
        remap[old_verts[ni]] = ni;
    }

    // Sao chép dữ liệu đỉnh
    sub.positions.resize(old_verts.size() * 3);
    sub.normals.resize(old_verts.size() * 3);
    sub.colors.resize(old_verts.size() * 3);
    for (size_t i = 0; i < old_verts.size(); ++i) {
        uint32_t ov = old_verts[i];
        sub.positions[i * 3 + 0] = mesh.vertices[ov * 3 + 0];
        sub.positions[i * 3 + 1] = mesh.vertices[ov * 3 + 1];
        sub.positions[i * 3 + 2] = mesh.vertices[ov * 3 + 2];

        if (ov * 3 + 2 < mesh.normals.size()) {
            sub.normals[i * 3 + 0] = mesh.normals[ov * 3 + 0];
            sub.normals[i * 3 + 1] = mesh.normals[ov * 3 + 1];
            sub.normals[i * 3 + 2] = mesh.normals[ov * 3 + 2];
        }
        if (ov * 3 + 2 < mesh.colors.size()) {
            sub.colors[i * 3 + 0] = mesh.colors[ov * 3 + 0];
            sub.colors[i * 3 + 1] = mesh.colors[ov * 3 + 1];
            sub.colors[i * 3 + 2] = mesh.colors[ov * 3 + 2];
        }
    }

    // Đánh lại số tam giác
    sub.indices.resize(face_indices.size() * 3);
    for (size_t i = 0; i < face_indices.size(); ++i) {
        uint32_t fi = face_indices[i];
        sub.indices[i * 3 + 0] = remap[mesh.indices[fi * 3 + 0]];
        sub.indices[i * 3 + 1] = remap[mesh.indices[fi * 3 + 1]];
        sub.indices[i * 3 + 2] = remap[mesh.indices[fi * 3 + 2]];
    }

    return sub;
}

// ──────────────────────────────────────────────────────────
//  Hàm phụ trợ: đưa lưới con vào cấu trúc glTF dạng node + mesh,
//  ghi thêm dữ liệu nhị phân vào buffer dùng chung.
// ──────────────────────────────────────────────────────────

static void add_gltf_mesh(tinygltf::Model& model,
                           std::vector<unsigned char>& buf,
                           const SubMesh& sub,
                           const std::string& node_name) {
    if (sub.indices.empty()) return;

    const size_t nv = sub.positions.size() / 3;
    const size_t ni = sub.indices.size();

    // ── Tính toán bao hộp AABB cho thuộc tính vị trí ───
    float minx = sub.positions[0], miny = sub.positions[1], minz = sub.positions[2];
    float maxx = minx, maxy = miny, maxz = minz;
    for (size_t i = 0; i < nv; ++i) {
        float px = sub.positions[i * 3 + 0];
        float py = sub.positions[i * 3 + 1];
        float pz = sub.positions[i * 3 + 2];
        if (px < minx) minx = px; if (px > maxx) maxx = px;
        if (py < miny) miny = py; if (py > maxy) maxy = py;
        if (pz < minz) minz = pz; if (pz > maxz) maxz = pz;
    }

    // ── Hàm phụ trợ: ghi dữ liệu thô vào buffer ───────
    auto append = [&](const void* data, size_t bytes) -> size_t {
        // Căn lề 4 byte
        while (buf.size() % 4 != 0) buf.push_back(0);
        size_t offset = buf.size();
        const auto* ptr = reinterpret_cast<const unsigned char*>(data);
        buf.insert(buf.end(), ptr, ptr + bytes);
        return offset;
    };

    int bv_base = static_cast<int>(model.bufferViews.size());
    int ac_base = static_cast<int>(model.accessors.size());

    // ── Ghi vị trí (POSITION) ──────────────────────────
    {
        size_t bytes = nv * 3 * sizeof(float);
        size_t off = append(sub.positions.data(), bytes);
        tinygltf::BufferView bv;
        bv.buffer     = 0;
        bv.byteOffset = off;
        bv.byteLength = bytes;
        bv.target     = TINYGLTF_TARGET_ARRAY_BUFFER;
        model.bufferViews.push_back(bv);

        tinygltf::Accessor acc;
        acc.bufferView    = bv_base + 0;
        acc.byteOffset    = 0;
        acc.componentType  = TINYGLTF_COMPONENT_TYPE_FLOAT;
        acc.count          = nv;
        acc.type           = TINYGLTF_TYPE_VEC3;
        acc.minValues      = {minx, miny, minz};
        acc.maxValues      = {maxx, maxy, maxz};
        model.accessors.push_back(acc);
    }

    // ── Ghi pháp tuyến (NORMAL) ────────────────────────
    {
        size_t bytes = nv * 3 * sizeof(float);
        size_t off = append(sub.normals.data(), bytes);
        tinygltf::BufferView bv;
        bv.buffer     = 0;
        bv.byteOffset = off;
        bv.byteLength = bytes;
        bv.target     = TINYGLTF_TARGET_ARRAY_BUFFER;
        model.bufferViews.push_back(bv);

        tinygltf::Accessor acc;
        acc.bufferView    = bv_base + 1;
        acc.byteOffset    = 0;
        acc.componentType  = TINYGLTF_COMPONENT_TYPE_FLOAT;
        acc.count          = nv;
        acc.type           = TINYGLTF_TYPE_VEC3;
        model.accessors.push_back(acc);
    }

    // ── Ghi màu sắc đỉnh (COLOR_0) ─────────────────────
    {
        size_t bytes = nv * 3 * sizeof(float);
        size_t off = append(sub.colors.data(), bytes);
        tinygltf::BufferView bv;
        bv.buffer     = 0;
        bv.byteOffset = off;
        bv.byteLength = bytes;
        bv.target     = TINYGLTF_TARGET_ARRAY_BUFFER;
        model.bufferViews.push_back(bv);

        tinygltf::Accessor acc;
        acc.bufferView    = bv_base + 2;
        acc.byteOffset    = 0;
        acc.componentType  = TINYGLTF_COMPONENT_TYPE_FLOAT;
        acc.count          = nv;
        acc.type           = TINYGLTF_TYPE_VEC3;
        model.accessors.push_back(acc);
    }

    // ── Ghi chỉ số (INDICES) ──────────────────────────
    {
        size_t bytes = ni * sizeof(uint32_t);
        size_t off = append(sub.indices.data(), bytes);
        tinygltf::BufferView bv;
        bv.buffer     = 0;
        bv.byteOffset = off;
        bv.byteLength = bytes;
        bv.target     = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
        model.bufferViews.push_back(bv);

        tinygltf::Accessor acc;
        acc.bufferView    = bv_base + 3;
        acc.byteOffset    = 0;
        acc.componentType  = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
        acc.count          = ni;
        acc.type           = TINYGLTF_TYPE_SCALAR;
        model.accessors.push_back(acc);
    }

    // ── Thiết lập Primitive ──────────────────────────
    tinygltf::Primitive prim;
    prim.attributes["POSITION"] = ac_base + 0;
    prim.attributes["NORMAL"]   = ac_base + 1;
    prim.attributes["COLOR_0"]  = ac_base + 2;
    prim.indices                = ac_base + 3;
    prim.mode                   = TINYGLTF_MODE_TRIANGLES;

    tinygltf::Mesh gltf_mesh;
    gltf_mesh.name = node_name;
    gltf_mesh.primitives.push_back(prim);
    int mesh_idx = static_cast<int>(model.meshes.size());
    model.meshes.push_back(gltf_mesh);

    // ── Thiết lập Node ───────────────────────────────
    tinygltf::Node node;
    node.name = node_name;
    node.mesh = mesh_idx;
    int node_idx = static_cast<int>(model.nodes.size());
    model.nodes.push_back(node);

    // Đưa node vào scene gốc
    model.scenes[0].nodes.push_back(node_idx);
}

// ──────────────────────────────────────────────────────────
//  API công khai
// ──────────────────────────────────────────────────────────

void GlbExporter::write(const MeshData& mesh,
                         const MeshLabels& labels,
                         const std::string& file_path) {
    std::cout << "[GLB_EXPORT] Đang chuẩn bị mô hình glTF..." << std::endl;

    tinygltf::Model model;
    model.asset.version   = "2.0";
    model.asset.generator = "3DGS-AR-Pipeline";

    // Khởi tạo một scene và một buffer duy nhất
    tinygltf::Scene scene;
    scene.name = "3DGS_AR_Scene";
    model.scenes.push_back(scene);
    model.defaultScene = 0;

    tinygltf::Buffer buffer;
    model.buffers.push_back(buffer);

    std::vector<unsigned char> bin_data;

    // ── Tạo các lưới con dựa trên nhãn phân loại AR ──────
    SubMesh occ_sub = extract_submesh(mesh, labels.occlusion_faces);
    SubMesh nav_sub = extract_submesh(mesh, labels.nav_faces);

    // Nếu không có nhãn phân loại, xuất toàn bộ lưới dưới một nút duy nhất
    if (occ_sub.indices.empty() && nav_sub.indices.empty()) {
        std::cout << "[GLB_EXPORT] Không có nhãn phân loại – xuất toàn bộ lưới làm một nút." << std::endl;
        std::vector<uint32_t> all_faces(mesh.indices.size() / 3);
        for (uint32_t i = 0; i < all_faces.size(); ++i) all_faces[i] = i;
        SubMesh full = extract_submesh(mesh, all_faces);
        add_gltf_mesh(model, bin_data, full, "FullMesh");
    } else {
        add_gltf_mesh(model, bin_data, occ_sub, "OcclusionMesh");
        add_gltf_mesh(model, bin_data, nav_sub, "NavMesh");
    }

    // Hoàn tất ghi buffer
    model.buffers[0].data = bin_data;

    // ── Ghi ra đĩa dưới định dạng file .glb nhị phân ──────
    tinygltf::TinyGLTF writer;
    bool ok = writer.WriteGltfSceneToFile(&model, file_path,
                                           /*embedImages=*/true,
                                           /*embedBuffers=*/true,
                                           /*prettyPrint=*/false,
                                           /*writeBinary=*/true);
    if (!ok) {
        throw std::runtime_error("Không thể ghi GLB ra file " + file_path);
    }

    std::cout << "[GLB_EXPORT] Đã ghi xong " << file_path
              << " (" << bin_data.size() << " byte dữ liệu buffer)." << std::endl;
}
