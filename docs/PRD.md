## MỤC TIÊU TỔNG QUAN

Xây dựng hệ thống wrapper cho thư viện rkllm với 2 thành phần chính:
- **io**: Lớp trừu tượng trực tiếp với rkllm  
- **nano**: Lớp giao tiếp với clients qua nhiều protocol

## THÀNH PHẦN 1: IO (Core Engine)

**Mục đích**: Lớp wrapper trực tiếp cho rkllm library
**Vị trí**: `src/libs/rkllm/` (rkllm.h + librkllmrt.so)

**Yêu cầu kỹ thuật**:
- Sử dụng C23 standard
- Ánh xạ 1:1 toàn bộ hàm rkllm, loại bỏ prefix "rkllm_"
- Thiết kế queue-based để:
  - Phục vụ đồng thời nhiều callers
  - Tránh crash hệ thống
  - Không thực hiện tính toán logic

**Kiến trúc hoạt động**:
```
Request Queue → RKLLM Processing → Response Queue
```

**Quản lý LLMHandle**:
- Ánh xạ LLMHandle* thành ID duy nhất
- Hỗ trợ đồng thời 2-3 models trên phần cứng mạnh
- Quản lý memory an toàn

## THÀNH PHẦN 2: NANO (Interface Layer)

**Mục đích**: Lớp giao tiếp đa protocol với clients
**Vị trí**: Trung gian giữa clients và io

**Protocols hỗ trợ**:
- UDP, TCP, HTTP, WebSocket, STDIO
- Tất cả tuân theo chuẩn MCP (JSON-based)

**Kiến trúc hoạt động**:
```
Clients → [UDP|TCP|HTTP|WS|STDIO] → NANO → IO → RKLLM
```

## THÁCH THỨC KỸ THUẬT CHÍNH

**Vấn đề**: MCP sử dụng JSON, nhưng RKLLM cần LLMHandle*

**Giải pháp**: 
- IO quản lý mapping: `LLMHandle* ↔ unique_id`
- Client chỉ biết `unique_id`, không truy cập trực tiếp LLMHandle
- Nano translate giữa JSON và C structs

## CẤU TRÚC DỮ LIỆU TEST

```
models/
├── qwenvl/model.rkllm          # Test model thường
└── lora/
    ├── model.rkllm             # Base model cho lora
    └── lora.rkllm              # Lora weights
```

## NGUYÊN TẮC THIẾT KẾ

1. **Minimal Code**: Tối thiểu LOC, tối đa hiệu quả  
3. **No Technical Debt**: Không mockup, implementation thật 100%

## WORKFLOW DEVELOPMENT

1. **Mục tiêu** → Xác định rõ requirements
2. **Nghiên cứu** → Phân tích rkllm API + MCP protocol  
3. **Kế hoạch** → Thiết kế architecture chi tiết
4. **Triển khai** → Code hoàn chỉnh trong một session
5. **Test** → Verify hoạt động thực tế với models