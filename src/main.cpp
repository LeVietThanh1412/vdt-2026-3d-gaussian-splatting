#include "CLI/CLI.hpp"
#include "core/constants.hpp"
#include "core/types.hpp"
#include "pipeline/optimization_pipeline.hpp"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    CLI::App app{constants::APP_NAME};

    // ── Cấu hình đường ống xử lý với các giá trị mặc định ──
    PipelineOptions opts;
    opts.filter.opacity_threshold = constants::DEFAULT_OPACITY_THRESHOLD;
    opts.filter.max_scale         = constants::DEFAULT_MAX_SCALE;
    opts.meshing.depth            = constants::DEFAULT_POISSON_DEPTH;
    opts.optimize.target_triangles = constants::DEFAULT_TARGET_TRIANGLES;
    opts.ar.max_slope_deg         = constants::DEFAULT_NAV_SLOPE_DEG;

    // Các tham số bắt buộc
    app.add_option("-i,--input",  opts.input_path,
                   "Đường dẫn file .ply đầu vào")->required();
    app.add_option("-o,--output", opts.output_path,
                   "Đường dẫn lưu file .glb đầu ra")->required();

    // Tùy chọn lọc nhiễu
    app.add_option("-p,--prune", opts.filter.opacity_threshold,
                   "Ngưỡng lọc độ mờ opacity (0.0 – 1.0)");
    app.add_option("--max-scale", opts.filter.max_scale,
                   "Tỷ lệ trục tối đa trước khi loại bỏ");
    app.add_option("--sor-k", opts.filter.knn_neighbours,
                   "Số láng giềng kNN cho thuật toán loại bỏ điểm ngoại lai (SOR)");
    app.add_option("--sor-std", opts.filter.std_ratio,
                   "Hệ số nhân độ lệch chuẩn cho SOR");
    app.add_flag("--no-sor", [&](int64_t) { opts.filter.enable_sor = false; },
                 "Tắt thuật toán loại bỏ điểm ngoại lai thống kê");

    // Tùy chọn tái tạo lưới
    app.add_option("-d,--depth", opts.meshing.depth,
                   "Độ sâu cây bát phân Poisson (càng cao càng chi tiết)");
    app.add_option("--density-quantile", opts.meshing.density_quantile,
                   "Ngưỡng mật độ cắt tỉa lưới (0.0 – 1.0)");

    // Tùy chọn rút gọn lưới
    app.add_option("-f,--faces", opts.optimize.target_triangles,
                   "Số tam giác mục tiêu sau khi rút gọn lưới");

    // Tùy chọn phân loại AR
    app.add_option("--nav-slope", opts.ar.max_slope_deg,
                   "Góc nghiêng tối đa (độ) cho lưới điều hướng");

    CLI11_PARSE(app, argc, argv);

    // ── Biển chào Banner ──
    std::cout << "\n";
    std::cout << "  _____  _____    _____  _____         ___   _____  \n";
    std::cout << " |____ ||  __ \\  / ____|/ ____|       / _ \\ |  __ \\ \n";
    std::cout << "     / /| |  | || |  __| (___ ______ | |_| || |__) |\n";
    std::cout << "     \\ \\| |  | || | |_ |\\___ \\______||  _  ||  _  / \n";
    std::cout << " .___/ /| |__| || |__| |____) |      | | | || | \\ \\ \n";
    std::cout << " \\____/ |_____/  \\_____|_____/       \\_| |_/\\_|  \\_\\ \n";
    std::cout << "\n";
    std::cout << "=======================================================\n";
    std::cout << "  STRUCTURAL OPTIMIZATION ENGINE v" << constants::VERSION << "\n";
    std::cout << "=======================================================\n";
    std::cout << " -> File đầu vào     : " << opts.input_path << "\n";
    std::cout << " -> File đầu ra      : " << opts.output_path << "\n";
    std::cout << " -> Ngưỡng lọc opacity: " << opts.filter.opacity_threshold << "\n";
    std::cout << " -> Tỷ lệ tối đa      : " << opts.filter.max_scale << "\n";
    std::cout << " -> Bật SOR          : " << (opts.filter.enable_sor ? "BẬT" : "TẮT") << "\n";
    std::cout << " -> Độ sâu Poisson    : " << opts.meshing.depth << "\n";
    std::cout << " -> Số tam giác mục tiêu: " << opts.optimize.target_triangles << "\n";
    std::cout << " -> Góc nghiêng tối đa: " << opts.ar.max_slope_deg << " độ\n";
    std::cout << "-------------------------------------------------------\n";

    try {
        OptimizationPipeline pipeline;
        pipeline.run(opts);

        std::cout << "\n[SUCCESS] Hoàn thành chạy đường ống xử lý thành công!\n";
    } catch (const std::exception& e) {
        std::cerr << "\n[FATAL ERROR] Lỗi nghiêm trọng dừng đường ống: " << e.what() << "\n";
        return 1;
    }

    return 0;
}