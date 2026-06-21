#pragma once

#include "core/types.hpp"

// ──────────────────────────────────────────────────────────
//  Tiện ích: in bảng thống kê kết quả đường ống ra stdout.
//  Được tách riêng khỏi OptimizationPipeline để tuân thủ
//  nguyên tắc Trách nhiệm Đơn lẻ (SRP) — đường ống điều
//  phối các giai đoạn, không phải định dạng đầu ra.
// ──────────────────────────────────────────────────────────
namespace stats_printer {

/// In bảng báo cáo kết quả đường ống ra stdout.
void print(const PipelineStats& stats);

} // namespace stats_printer
