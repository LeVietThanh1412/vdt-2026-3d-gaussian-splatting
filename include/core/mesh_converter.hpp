#pragma once

#include "core/types.hpp"

#include <open3d/Open3D.h>

#include <memory>

//  Tiện ích: chuyển đổi hai chiều giữa cấu trúc MeshData
//  dạng mảng phẳng và đối tượng TriangleMesh của Open3D.
//  Giúp loại bỏ mã trùng lặp giữa mesh_generator và
//  mesh_optimizer.
namespace mesh_converter {

    /// Chuyển đổi MeshData (mảng phẳng) → TriangleMesh của Open3D.
    /// Sao chép đỉnh, màu đỉnh, pháp tuyến và chỉ số tam giác.
    std::shared_ptr<open3d::geometry::TriangleMesh>
    to_open3d(const MeshData& mesh);

    /// Chuyển đổi TriangleMesh của Open3D → MeshData (mảng phẳng).
    /// Sao chép đỉnh, pháp tuyến, màu đỉnh và chỉ số tam giác.
    MeshData from_open3d(const open3d::geometry::TriangleMesh& o3d_mesh);

} // namespace mesh_converter
