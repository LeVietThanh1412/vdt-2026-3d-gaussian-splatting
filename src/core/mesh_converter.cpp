#include "core/mesh_converter.hpp"

#include <cstdint>

// ──────────────────────────────────────────────────────────
//  Chuyển đổi MeshData (mảng phẳng) → TriangleMesh của Open3D
// ──────────────────────────────────────────────────────────

std::shared_ptr<open3d::geometry::TriangleMesh>
mesh_converter::to_open3d(const MeshData& mesh) {
    auto o3d = std::make_shared<open3d::geometry::TriangleMesh>();

    const size_t nv = mesh.vertices.size() / 3;  // Số đỉnh
    const size_t nf = mesh.indices.size() / 3;    // Số mặt tam giác

    // Sao chép tọa độ đỉnh
    o3d->vertices_.resize(nv);
    for (size_t i = 0; i < nv; ++i) {
        o3d->vertices_[i] = Eigen::Vector3d(
            mesh.vertices[i * 3 + 0],
            mesh.vertices[i * 3 + 1],
            mesh.vertices[i * 3 + 2]);
    }

    // Sao chép màu sắc đỉnh (nếu có)
    if (mesh.colors.size() >= nv * 3) {
        o3d->vertex_colors_.resize(nv);
        for (size_t i = 0; i < nv; ++i) {
            o3d->vertex_colors_[i] = Eigen::Vector3d(
                mesh.colors[i * 3 + 0],
                mesh.colors[i * 3 + 1],
                mesh.colors[i * 3 + 2]);
        }
    }

    // Sao chép pháp tuyến đỉnh (nếu có)
    if (mesh.normals.size() >= nv * 3) {
        o3d->vertex_normals_.resize(nv);
        for (size_t i = 0; i < nv; ++i) {
            o3d->vertex_normals_[i] = Eigen::Vector3d(
                mesh.normals[i * 3 + 0],
                mesh.normals[i * 3 + 1],
                mesh.normals[i * 3 + 2]);
        }
    }

    // Sao chép chỉ số tam giác
    o3d->triangles_.resize(nf);
    for (size_t i = 0; i < nf; ++i) {
        o3d->triangles_[i] = Eigen::Vector3i(
            static_cast<int>(mesh.indices[i * 3 + 0]),
            static_cast<int>(mesh.indices[i * 3 + 1]),
            static_cast<int>(mesh.indices[i * 3 + 2]));
    }

    return o3d;
}

// ──────────────────────────────────────────────────────────
//  Chuyển đổi TriangleMesh của Open3D → MeshData (mảng phẳng)
// ──────────────────────────────────────────────────────────

MeshData mesh_converter::from_open3d(
        const open3d::geometry::TriangleMesh& o3d_mesh) {
    MeshData out;
    const size_t nv = o3d_mesh.vertices_.size();  // Số đỉnh
    const size_t nf = o3d_mesh.triangles_.size();  // Số mặt tam giác

    // Cấp phát bộ nhớ cho tất cả mảng
    out.vertices.resize(nv * 3);
    out.normals.resize(nv * 3);
    out.colors.resize(nv * 3);
    out.indices.resize(nf * 3);

    // Sao chép tọa độ đỉnh
    for (size_t i = 0; i < nv; ++i) {
        out.vertices[i * 3 + 0] = static_cast<float>(o3d_mesh.vertices_[i].x());
        out.vertices[i * 3 + 1] = static_cast<float>(o3d_mesh.vertices_[i].y());
        out.vertices[i * 3 + 2] = static_cast<float>(o3d_mesh.vertices_[i].z());
    }

    // Sao chép pháp tuyến đỉnh
    for (size_t i = 0; i < nv && i < o3d_mesh.vertex_normals_.size(); ++i) {
        out.normals[i * 3 + 0] = static_cast<float>(o3d_mesh.vertex_normals_[i].x());
        out.normals[i * 3 + 1] = static_cast<float>(o3d_mesh.vertex_normals_[i].y());
        out.normals[i * 3 + 2] = static_cast<float>(o3d_mesh.vertex_normals_[i].z());
    }

    // Sao chép màu sắc đỉnh
    for (size_t i = 0; i < nv && i < o3d_mesh.vertex_colors_.size(); ++i) {
        out.colors[i * 3 + 0] = static_cast<float>(o3d_mesh.vertex_colors_[i].x());
        out.colors[i * 3 + 1] = static_cast<float>(o3d_mesh.vertex_colors_[i].y());
        out.colors[i * 3 + 2] = static_cast<float>(o3d_mesh.vertex_colors_[i].z());
    }

    // Sao chép chỉ số tam giác
    for (size_t i = 0; i < nf; ++i) {
        out.indices[i * 3 + 0] = static_cast<uint32_t>(o3d_mesh.triangles_[i].x());
        out.indices[i * 3 + 1] = static_cast<uint32_t>(o3d_mesh.triangles_[i].y());
        out.indices[i * 3 + 2] = static_cast<uint32_t>(o3d_mesh.triangles_[i].z());
    }

    return out;
}
