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



========================================





Cần phải viết ngay 1 lọat test dành riêng cho IO, và một loạt test dành riêng cho NANO. Để đảm bảo IO và NANO phải hoạt động theo như đúng thiết kế. 

HIỆN NAY ĐANG CÓ CÁC VẤN ĐỀ:

Phát Hiện Quan Trọng: IO Layer Không Được Sử Dụng

**QUAN TRỌNG**: Theo phân tích code, hệ thống hiện tại **KHÔNG SỬ DỤNG** IO layer với queue như đã thiết kế:

- `nano_process_message()` gọi trực tiếp `rkllm_proxy_execute()`
- `rkllm_proxy_execute()` gọi trực tiếp các operation handlers
- Operation handlers gọi trực tiếp RKLLM functions
- **KHÔNG CÓ** `io_push_request()` hay `io_pop_response()` nào được sử dụng

```c
// Trong nano.c
int nano_process_message(const mcp_message_t* request, mcp_message_t* response) {
    // Tạo rkllm_request_t
    rkllm_request_t rkllm_request = {...};
    
    // Gọi trực tiếp RKLLM proxy
    int ret = rkllm_proxy_execute(&rkllm_request, &rkllm_result);
    
    // Trả response
    return 0;
}
```

IO Có vẻ như cần được thiết kế lại? Hay là NANO đang cần được thiết kế lại?
Mọi thứ phải đúng như PLAN, PRD, RULES 
analysis.md đang cho thấy rất nhiều vấn đề nghiêm trọng!!! IO và NANO đang không hoạt động đúng như thiết kế! Cần sửa ngay!