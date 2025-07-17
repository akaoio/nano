Kiểm tra Codebase
Mục tiêu: Đánh giá tình trạng kiến trúc, nợ kỹ thuật và quản lý bộ nhớ trên hệ thống.
Bước 1: Đọc Tài liệu & Chuẩn bị
Đọc kỹ PRD.md, RULES.md, và PLAN.md để nắm rõ mục tiêu, quy tắc và kiến trúc dự định của dự án. Điều này rất quan trọng để đánh giá sự "trôi dạt kiến trúc" và các quyết định kỹ thuật.
Bước 2: Phân tích Codebase
Thực hiện kiểm tra mã nguồn chuyên sâu:
1. Trôi dạt Kiến trúc & Nợ Kỹ thuật
 * Trôi dạt Kiến trúc: So sánh cấu trúc code hiện tại (bao gồm cách các module IO và NANO tương tác) với kiến trúc dự định trong tài liệu. Tìm các đoạn code phá vỡ quy tắc, gây khó hiểu hoặc đi chệch khỏi thiết kế ban đầu.
 * Nợ Kỹ thuật:
   * Tìm kiếm tất cả các ghi chú: "skip", "for now", "simplified", "deprecated", "todo". Liệt kê và đánh giá mức độ nghiêm trọng.
   * Phát hiện các phần code phức tạp, khó đọc, thiếu comment hoặc có nguy cơ gây lỗi.
   * Kiểm tra sự tuân thủ các quy tắc mã hóa trong RULES.md.
2. Mã Thừa & Không sử dụng
 * Mã trùng lặp: Xác định các đoạn code giống nhau.
 * Mã mồ côi/chết: Tìm các file, hàm, lớp không được gọi/sử dụng, hoặc code không bao giờ được thực thi.

Sau khi nghiên cứu, hãy tổng hợp kết quả vào một báo cáo ngắn gọn, tập trung vào các điểm chính đã tìm thấy.
Generate report.md




Code Quality Issues

  🚨 Significant Code Duplication
  (25+ instances)

  1. JSON Parsing Logic - 25+
  duplications
  // Pattern repeated across
  rkllm_operations.c
  const char* field_start =
  strstr(params_json, "\"field\":");
  if (field_start) {
      field_start =
  strchr(field_start + 8, '"');
      // ... extraction logic
  }

  2. Error Handling Patterns - 30+
  duplications
  result->result_data =
  rkllm_proxy_create_error_result(-1,
   "Error message");
  result->result_size =
  strlen(result->result_data);

  3. Handle Validation - 13
  duplications
  LLMHandle handle =
  rkllm_proxy_get_handle(handle_id);
  if (!handle) {
      result->result_data =
  rkllm_proxy_create_error_result(-1,
   "Invalid handle");
      return -1;
  }

  4. Transport Buffer Processing - 4
  duplications across transport files

  🧹 Cleanup Recommendations

  - High Priority: Consolidate JSON
  parsing into utility functions
  - Medium Priority: Create error
  handling macros
  - Low Priority: Extract common
  validation patterns