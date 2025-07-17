# PIPELINE PHÁT TRIỂN VÀ CÁC NGUYÊN TẮC KỸ THUẬT

## 1. Đánh giá tuân thủ kiến trúc
**Mục tiêu**: Thực hiện phân tích so sánh giữa kiến trúc hiện tại và đặc tả kỹ thuật đã định nghĩa.

**Output**: Báo cáo trạng thái implementation với danh sách các thành phần đã hoàn thành và còn thiếu.

## 2. Chính sách loại bỏ nợ kỹ thuật
**Nguyên tắc**: Zero tolerance với technical debt trong production code.

**Định nghĩa nợ kỹ thuật**: Bất kỳ code pattern nào thuộc các loại:
- Mock implementations, placeholder functions, TODO comments
- Temporary solutions ("for now", "simplified") 
- Incomplete implementations

**Enforcement**: NGHIÊM CẤM MOCK UP, TODO, PLACEHOLDER, SIMPLIFIED - KHÔNG CÓ NGOẠI LỆ.

## 3. Phân tích phân chia trách nhiệm component
**Scope**: Thực hiện kiểm tra boundary verification cho modules `io` và `nano`.

**Tiêu chí**: Đảm bảo không có overlap chức năng hoặc vi phạm coupling theo đặc tả kiến trúc.

## 4. Chuẩn hóa hệ thống build
**Protocol**: Centralized build pipeline với output consolidation.

**Triển khai**:
```
Source → build/ → {io, nano, test} → root/
```

**Yêu cầu**: Tất cả artifacts phải được generate qua unified build process.

## 5. Protocol đồng bộ test-code
**Nguyên tắc**: Duy trì sự đồng bộ giữa test và implementation.

**Yêu cầu**:
- Test suites chỉ được reference các function hiện có
- Bắt buộc update test khi có code changes
- Functional coverage phải phản ánh đúng implementation state

## 6. Protocol ngăn chặn trùng lặp
**Kiểm tra tiền tạo**: Bắt buộc scan codebase trước khi tạo mới:
- Files, functions, classes/structs, data structures

**Quy tắc**: Không tạo lại bất kỳ thành phần đã tồn tại.

## 7. Ràng buộc mật độ code
**Giới hạn**: Tối đa 100 dòng mỗi file (bao gồm comments).

**Lý do**: Ép buộc intelligent design thay vì brute-force implementations.

**Enforcement**: Tự động phát hiện vi phạm - BẠN SẼ BỊ TẮT nếu vi phạm.

## 8. Đặc tả hierarchy dependency
**Formal dependency graph**:
```
rkllm → io → nano → tests
```

**Quy tắc đồng bộ**:
- `io` implementation phải khớp với `rkllm` API specifications
- `nano` implementation phải derive từ `io` interface  
- `tests` phải validate riêng biệt `io` và `nano` functionality
- Isolation requirement: Test suites riêng cho từng component
- Change propagation: Code modifications trigger mandatory test updates