# 3DGS AR Pipeline (Bộ Tối Ưu Hóa Cấu Trúc Hình Học)

> **Đề tài:** Nghiên cứu, phát triển công cụ xử lý dữ liệu 3D Gaussian Splatting phục vụ hệ thống Thực tế tăng cường (AR)

Dự án này cung cấp một công cụ dòng lệnh (CLI) hiệu năng cao được viết bằng **C++17**, tích hợp thư viện **Open3D** để xử lý và chuyển đổi dữ liệu đám mây điểm **3D Gaussian Splatting (3DGS)** hoặc **Point Cloud thông thường** thành mô hình lưới 3D có cấu trúc hình học (`.glb`). Lưới thành phẩm được tối ưu hóa cao, tự động phân loại thành hai nhóm riêng biệt: **Lưới che khuất (Occlusion Mesh)** và **Lưới điều hướng (Navigation Mesh)** để tích hợp trực tiếp vào các môi trường ứng dụng Thực tế ảo/Thực tế tăng cường (AR/VR).

---

## Các Tính Năng Nổi Bật

| Tính năng | Mô tả chi tiết |
|---|---|
| **Đọc Dữ Liệu Đa Dạng** | Hỗ trợ đọc cả dữ liệu 3DGS chuyên biệt (chuyển đổi hệ số màu SH DC, kích hoạt độ mờ sigmoid, quaternion xoay) lẫn Point Cloud thô dạng màu RGB từ uchar/double. |
| **Tiền Xử Lý & Lọc Nhiễu** | Lọc bỏ hạt lỗi (NaN), hạt mờ (ngưỡng opacity thấp), hạt có tỷ lệ biến dạng quá lớn, và loại bỏ điểm ngoại lai thống kê bằng **SOR (Statistical Outlier Removal)**. |
| **Tái Tạo Lưới Poisson** | Xây dựng lưới bề mặt liền mạch thông qua thuật toán **Poisson Surface Reconstruction** và cắt tỉa vùng biên có mật độ thấp bằng phân vị mật độ (density quantile). |
| **Tối Ưu Hóa Lưới Tam Giác** | Rút gọn số lượng tam giác thông qua **Quadric Error Decimation**, đồng thời dọn dẹp các tam giác suy biến và đỉnh trùng lặp để lưới cực kỳ nhẹ (chỉ khoảng 2MB cho AR). |
| **Phân Loại Cấu Trúc AR** | Phân tích hướng pháp tuyến (normal) của từng mặt tam giác so với trục thẳng đứng (+Y) để tách thành lưới di chuyển được (NavMesh) và lưới cản góc nhìn (OcclusionMesh). |
| **Xuất Mô Hình glTF nhị phân** | Tạo file `.glb` chuẩn hóa gồm 2 nút (node) riêng biệt, sẵn sàng nhúng trực tiếp vào các thư viện web/mobile. |
| **Công Cụ Web Tương Tác** | Demo trực quan dựa trên `<model-viewer>` hỗ trợ chế độ xem AR bằng Camera điện thoại thực tế qua LAN. |

---

## Kiến Trúc Hệ Thống & Tuân Thủ SOLID

Mã nguồn dự án được thiết kế tỉ mỉ, tuân thủ các nguyên tắc thiết kế sạch:

1. **Trách nhiệm Đơn lẻ (SRP):** Tách biệt rõ ràng phần đọc ghi (`PlyReader`, `GlbExporter`), lọc (`GsplatFilter`), tạo lưới (`MeshGenerator`), rút gọn (`MeshOptimizer`), phân loại (`ArExtractor`) và in kết quả (`stats_printer`).
2. **Không ép buộc kế thừa ảo:** Loại bỏ lớp cơ sở rỗng `BaseProcessor` để tránh đa hình giả (Fake Polymorphism), tăng tính dễ hiểu và tối ưu hiệu suất biên dịch.
3. **Tránh trùng lặp mã (DRY):** Tập trung toàn bộ logic chuyển đổi kiểu dữ liệu phẳng `MeshData` ↔ `open3d::geometry::TriangleMesh` vào tiện ích chung `mesh_converter`.
4. **Phân tách Interface (ISP):** Các lớp xử lý chỉ nhận các cấu hình độc lập (`FilterOptions`, `MeshingOptions`, etc.) giúp bảo đảm không phụ thuộc chéo.

```
                  ┌──────────────┐
                  │  Input .ply  │
                  └──────┬───────┘
                         │ (PlyReader - Stage 1)
                         ▼
             ┌───────────────────────┐
             │  vector<GsplatSplat>  │
             └───────────┬───────────┘
                         │ (GsplatFilter - Stage 2)
                         ▼
             ┌───────────────────────┐
             │   Filtered Splats     │
             └───────────┬───────────┘
                         │ (MeshGenerator - Stage 3)
                         ▼
                  ┌──────────────┐
                  │  MeshData    │
                  └──────┬───────┘
                         │ (MeshOptimizer - Stage 4)
                         ▼
                  ┌──────────────┐
                  │  MeshData    │
                  └──────┬───────┘
                         │ (ArExtractor - Stage 5)
                         ▼
                  ┌──────────────┐
                  │  MeshLabels  │ (Phân loại Nav/Occlusion)
                  └──────┬───────┘
                         │ (GlbExporter - Stage 6)
                         ▼
                  ┌──────────────┐
                  │ Output .glb  │
                  └──────────────┘
```

---

## Cấu Trúc Thư Mục Dự Án

```
3dgs-ar-pipeline/
.
├── CMakeLists.txt
├── vcpkg
├── vcpkg-installed
├── build
├── data
│   ├── input
│   │   ├── ChristmasTree.ply
│   │   └── flowers.ply
│   └── output
│       ├── ChristmasTree.glb
│       └── flowers.glb
├── docker-compose.yml
├── Dockerfile
├── include
│   ├── core
│   │   ├── constants.hpp
│   │   ├── mesh_converter.hpp
│   │   ├── stats_printer.hpp
│   │   └── types.hpp
│   ├── io
│   │   ├── glb_exporter.hpp
│   │   └── ply_reader.hpp
│   ├── pipeline
│   │   └── optimization_pipeline.hpp
│   └── processor
│       ├── ar_extractor.hpp
│       ├── gsplat_filter.hpp
│       ├── mesh_generator.hpp
│       └── mesh_optimizer.hpp
├── LICENSE
├── plans
│   ├── 01_dependencies.md
│   ├── 02_structure.md
│   ├── 03_core_headers.md
│   ├── 04_src_impl.md
│   ├── 05_web_demo.md
│   ├── 06_scripts.md
│   ├── 07_algorithm_details.md
│   └── technology_used.md
├── README.md
├── scripts
│   ├── bootstrap_vcpkg.sh
│   ├── build.sh
│   ├── fetch_third_party.sh
│   └── run.sh
├── src
│   ├── core
│   │   ├── mesh_converter.cpp
│   │   └── stats_printer.cpp
│   ├── io
│   │   ├── glb_exporter.cpp
│   │   └── ply_reader.cpp
│   ├── main.cpp
│   ├── pipeline
│   │   └── optimization_pipeline.cpp
│   └── processor
│       ├── ar_extractor.cpp
│       ├── gsplat_filter.cpp
│       ├── mesh_generator.cpp
│       └── mesh_optimizer.cpp
├── tests
│   ├── test_filter.cpp
│   └── test_io.cpp
├── third_party
│   ├── happly
│   │   └── happly.h
│   └── tinygltf
│       ├── json.hpp
│       ├── stb_image.h
│       ├── stb_image_write.h
│       └── tiny_gltf.h
├── vcpkg_installed
├── vcpkg.json
└── web_demo
    ├── ChristmasTree.glb
    ├── index.html
    ├── README.md
    └── serve.py

22 directories, 54 files
```

---

## Hướng Dẫn Cài Đặt & Biên Dịch (Build)

### Yêu Cầu Hệ Thống Cần Thiết
- Hệ điều hành Linux (khuyến nghị Ubuntu 20.04+ hoặc Arch Linux)
- Trình biên dịch hỗ trợ C++17 (GCC 9+ hoặc Clang 10+)
- CMake bản 3.15 trở lên

---

### Hướng Dẫn Biên Dịch Chi Tiết

#### Bước 1: Cài đặt các công cụ biên dịch cơ bản
```bash
# Trên Ubuntu/Debian:
sudo apt update && sudo apt install build-essential cmake git curl zip unzip tar pkg-config

# Trên Arch Linux:
sudo pacman -S base-devel cmake git curl zip unzip tar pkgconf
```

#### Bước 2: Tải gói Open3D C++ SDK
Tải xuống gói Open3D C++ pre-built để tránh mất nhiều tiếng đồng hồ tự build từ mã nguồn:
```bash
cd ~
curl -L https://github.com/isl-org/Open3D/releases/download/v0.19.0/open3d-devel-linux-x86_64-cxx11-abi-0.19.0.tar.xz -o open3d.tar.xz
tar -xf open3d.tar.xz
# Xuất đường dẫn môi trường hoặc copy đường dẫn này
export OPEN3D_ROOT=~/open3d-devel-linux-x86_64-cxx11-abi-0.19.0
```

#### Bước 3: Cài đặt thư viện phân tích dòng lệnh `CLI11`
Bạn có thể cài đặt trực tiếp thông qua quản lý gói hệ thống:
```bash
# Trên Ubuntu:
sudo apt install libcli11-dev

# Trên Arch Linux:
sudo pacman -S cli11
```
*(Nếu hệ thống không có sẵn gói, dự án sẽ tự động dùng môi trường `vcpkg` khi bạn chạy `./scripts/bootstrap_vcpkg.sh`)*.

#### Bước 4: Tải các thư viện dạng Header-only
Tự động tải các gói `happly` (đọc PLY) và `tinygltf` (ghi GLB) về thư mục local:
```bash
./scripts/fetch_third_party.sh
```

#### Bước 5: Thực hiện Biên Dịch (Build Project)
Chạy script build chính:
```bash
./scripts/build.sh
```

> [!WARNING]
> **Lưu ý về lỗi treo hệ thống trên phân vùng NTFS:**
> Trình điều khiển (driver) `ntfs3` của nhân Linux thường xuyên bị treo cứng (deadlock) khi ghi file ghi đè song song với tiến trình build C++ lớn.
> Script `build.sh` của dự án đã tích hợp tính năng **tự động phát hiện NTFS** và chuyển toàn bộ file tạm sinh ra trong lúc build sang phân vùng Linux native (`/home/archlinux/.cache/3dgs_ar_build`), sau đó chỉ copy file nhị phân thành phẩm về lại thư mục dự án. Điều này giúp ngăn ngừa 100% tình trạng đứng máy.

Sau khi quá trình biên dịch hoàn tất, file nhị phân `gsplat_ar` sẽ nằm tại:
- Thực tế build: `/home/archlinux/.cache/3dgs_ar_build/gsplat_ar`
- Liên kết copy tại dự án: `./build/gsplat_ar`

---

## Hướng Dẫn Chạy Đường Ống Xử Lý (Run)

Dự án cung cấp 2 cách để chạy chương trình: qua script tương tác rất dễ dùng, hoặc chạy dòng lệnh thủ công có tùy chỉnh sâu.

### Cách 1: Chạy qua Script tương tác (Khuyến nghị cho người mới)
Gõ lệnh sau:
```bash
./scripts/run.sh
```
Hệ thống sẽ:
1. Quét toàn bộ thư mục `data/input/` và hiển thị danh sách các file `.ply` kèm dung lượng chi tiết.
2. Cho phép bạn gõ số thứ tự (ví dụ: `1`, `2`) để chọn file cần chạy.
3. Hiển thị thông số mặc định của đường ống.
4. Tự động gọi file thực thi và lưu kết quả `.glb` vào thư mục `data/output/` trùng tên file gốc.

---

### Cách 2: Chạy thủ công bằng dòng lệnh CLI (Tùy chỉnh sâu)
Để có toàn quyền kiểm soát các tham số của từng thuật toán:

```bash
./build/gsplat_ar \
  -i data/input/flowers.ply \
  -o data/output/flowers.glb \
  -p 0.15 \
  -d 9 \
  --density-quantile 0.05 \
  -f 60000 \
  --nav-slope 30.0
```

#### Giải thích ý nghĩa các tham số cốt lõi:
- `-i, --input`: Đường dẫn file `.ply` đầu vào cần xử lý (bắt buộc).
- `-o, --output`: Đường dẫn lưu file `.glb` kết quả đầu ra (bắt buộc).
- `-p, --prune`: Ngưỡng lọc hạt có độ mờ opacity thấp. Giá trị từ `0.0` đến `1.0`. Các hạt có độ mờ dưới ngưỡng này sẽ bị loại bỏ sớm để làm sạch nhiễu nền xung quanh vật thể.
- `-d, --depth`: Độ sâu octree của thuật toán Poisson Surface Reconstruction (mặc định: `9`). Độ sâu càng lớn thì lưới tam giác tạo ra càng chi tiết nhưng tiêu tốn bộ nhớ và thời gian chạy lâu hơn. (Thường đặt từ `6` đến `10`).
- `--density-quantile`: Giá trị phân vị mật độ đỉnh dùng để xén tỉa lưới. Giá trị `0.05` nghĩa là xén đi 5% số lượng đỉnh nằm ở các khu vực rìa thưa thớt hạt để lưới gọn gàng.
- `-f, --faces`: Số tam giác mục tiêu sau khi rút gọn lưới bằng thuật toán Quadric Decimation. Giúp giảm dung lượng file xuống hàng chục lần mà vẫn giữ nguyên hình dạng.
- `--nav-slope`: Góc nghiêng tối đa (độ) so với phương ngang. Các mặt tam giác có độ dốc thấp hơn góc này sẽ được dán nhãn là lưới điều hướng (`NavMesh`), phục vụ nhân vật AR di chuyển.

---

### Các tham số dòng lệnh chi tiết bổ sung
| Tham số | Mặc định | Ý nghĩa |
|---|---|---|
| `--max-scale` | `10.0` | Loại bỏ các hạt Gaussian có tỷ lệ co giãn bất thường lớn hơn giá trị này (thường là hạt nhiễu). |
| `--sor-k` | `30` | Số lượng láng giềng kNN phục vụ việc tính khoảng cách trung bình lọc điểm ngoại lai thống kê. |
| `--sor-std` | `2.0` | Hệ số độ lệch chuẩn để loại bỏ điểm nằm xa đám đông hạt cốt lõi. |
| `--no-sor` | *(Bật mặc định)* | Thêm flag này nếu muốn tắt thuật toán SOR giúp tăng tốc xử lý khi dữ liệu đầu vào đã rất sạch. |

---

## Xem Mô Hình Kết Quả Trực Quan Trên Web Demo

Thư mục `web_demo/` chứa một ứng dụng web nhẹ được phát triển bằng HTML5 sử dụng `<model-viewer>` của Google, cho phép bạn xoay, phóng to, thu nhỏ mô hình và thử nghiệm tính năng AR.

1. **Khởi động server quét tự động:**
   ```bash
   python3 web_demo/serve.py
   ```
2. **Xem trên máy tính local:**
   Mở trình duyệt web và truy cập địa chỉ: `http://localhost:8000`. Server sẽ tự động liệt kê các file `.glb` trong `data/output/` để bạn xem nhanh mà không cần copy thủ công.
3. **Xem AR trên điện thoại di động thực tế:**
   Đảm bảo điện thoại và máy tính chạy server kết nối chung một mạng Wi-Fi (LAN). Truy cập địa chỉ `http://<IP_MÁY_TÍNH>:8000` trên điện thoại di động của bạn, bấm vào nút **"View in AR"** để xem mô hình hiển thị trực tiếp trong phòng thực tế.

Bạn có thể dùng https://gltf-viewer.donmccurdy.com/ để xem mô hình, bằng cách kéo thả file glb vào trình duyệt.

---

## Chạy Bằng Docker (Cách Thay Thế)

Nếu bạn không muốn cài đặt CMake hay tải SDK Open3D trên máy cục bộ, bạn có thể đóng gói tất cả vào container Docker:

```bash
# 1. Biên dịch dự án và chạy tối ưu hóa tự động theo cấu hình file yaml
docker compose up builder

# 2. Khởi động máy chủ Web Demo để truy cập qua cổng 8000
docker compose up web_demo
```