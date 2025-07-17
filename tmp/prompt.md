Bạn đúng, tôi xin lỗi vì đã hiểu sai ngữ cảnh về "IO" và "NANO" trong prompt trước. Với thông tin rõ ràng hơn về môi trường của bạn (Orange Pi 5 Plus, ARM64, RK3588, NPU, 32GB RAM) và cách bạn gọi tên các thành phần trong code ("IO", "NANO"), tôi sẽ viết lại một cách ngắn gọn, tập trung vào các yêu cầu cốt lõi và chính xác hơn.
PROMPT 1: Kiểm tra Codebase cho Orange Pi 5 Plus (RK3588 NPU)
Mục tiêu: Đánh giá tình trạng kiến trúc, nợ kỹ thuật và quản lý bộ nhớ trên hệ thống Orange Pi 5 Plus của bạn.
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
Bước 3: Đánh giá Quản lý Bộ nhớ (Đặc biệt cho RK3588 NPU)
Với RK3588 NPU và 32GB RAM, việc quản lý bộ nhớ rất quan trọng.
1. Cơ chế Quản lý Bộ nhớ
 * Kiểm tra cách ứng dụng cấp phát/giải phóng bộ nhớ trên RAM 32GB của Orange Pi 5 Plus.
 * Đối với NPU (RK3588):
   * Tìm hiểu cách các model AI, tensor và dữ liệu được cấp phát/giải phóng trên bộ nhớ của NPU.
   * Xác định cơ chế quản lý bộ nhớ của lib rkllm.h va librkllmrt.so trên NPU.
   * Kiểm tra liệu IO và NANO có tương tác với bộ nhớ NPU một cách hiệu quả không.
2. Tối ưu & Mở rộng Bộ nhớ NPU
 * Real life scenario:
   * When no model loaded, memory is not yet allocated for NPU, OK.
   * When the first model loaded, memory be allocated for NPU according to the model size. OK if we only have one model. But we want to be able to load more models because our machine is quite powerful.
   * When second model loads -> fail, why, NPU memory is not enough. But actually NPU uses memory from RAM, and my RAM is 32GB. The problem is we don't know how to make memory flexible yet.
 * Find a solution: Tăng bộ nhớ NPU đã cấp phát: "tăng thêm" bộ nhớ cho NPU đã cấp phát dynamically. Possible or not?
 * If possible, how?
 * If not possible, then do we have other choices?

Sau khi nghiên cứu, hãy tổng hợp kết quả vào một báo cáo ngắn gọn, tập trung vào các điểm chính đã tìm thấy.
