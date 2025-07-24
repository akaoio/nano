# JSON Parameter Adapter System - Developer Guide

**Date**: 2025-07-24  
**Status**: IMPLEMENTED  
**Scope**: Unified JSON-RPC Interface for RKNN and RKLLM APIs

## Vấn đề đã giải quyết

### 1. **JSON Functions phân tán**
- **Trước**: Các hàm JSON conversion chỉ có trong `src/rkllm/`, RKNN không thể sử dụng
- **Sau**: Tạo `src/jsonrpc/json_param_adapter/` - module chung cho cả RKNN và RKLLM

### 2. **Cấu trúc JSON-RPC không nhất quán**
- **Trước**: RKLLM dùng array `[null, {...}, {...}, null]`, RKNN dùng object `{model_path: "..."}`
- **Sau**: Hệ thống adapter tự động chuẩn hóa - client có thể dùng cả hai format

## Kiến trúc JSON Parameter Adapter

```
src/jsonrpc/json_param_adapter/
├── json_param_adapter.h     # Interface definitions
└── json_param_adapter.c     # Implementation

Core Functions:
├── create_unified_params()    # Tạo unified parameter structure
├── normalize_rknn_params()    # Chuẩn hóa RKNN params → object format
├── normalize_rkllm_params()   # Chuẩn hóa RKLLM params → array format  
├── validate_rknn_params()     # Validate RKNN parameters
├── validate_rkllm_params()    # Validate RKLLM parameters
└── extract_*_param()          # Helper functions tái sử dụng
```

## API Sử dụng thống nhất cho Developers

### RKNN API - Clients có thể dùng cả hai format

#### Format 1: Object (khuyến khích - dễ hiểu)
```json
{
  "jsonrpc": "2.0",
  "method": "rknn.init",
  "params": {
    "model_path": "/models/vision.rknn",
    "flag": 0
  },
  "id": 1
}
```

#### Format 2: Array (backward compatibility)
```json
{
  "jsonrpc": "2.0", 
  "method": "rknn.init",
  "params": ["/models/vision.rknn", 0],
  "id": 1
}
```

**Cả hai format đều được chuyển đổi thành format object thống nhất bên trong server.**

### RKLLM API - Clients có thể dùng cả hai format

#### Format 1: Object (user-friendly - khuyến khích)
```json
{
  "jsonrpc": "2.0",
  "method": "rkllm.run",
  "params": {
    "prompt": "Hello, how are you?",
    "temperature": 0.7,
    "max_tokens": 256
  },
  "id": 1
}
```

#### Format 2: Array (RKLLM native format)
```json
{
  "jsonrpc": "2.0",
  "method": "rkllm.run", 
  "params": [
    null,
    {
      "input_type": 0,
      "prompt_input": "Hello, how are you?",
      "role": "user",
      "enable_thinking": false
    },
    {
      "mode": 0,
      "lora_params": null,
      "prompt_cache_params": null,
      "keep_history": 0
    },
    null
  ],
  "id": 1
}
```

**Cả hai format đều được chuyển đổi thành format array chuẩn RKLLM bên trong server.**

## Flow xử lý trong Server

```
1. Client gửi request (object hoặc array format)
      ↓
2. JSON Parameter Adapter nhận params
      ↓
3. create_unified_params() phân tích method + params
      ↓
4. normalize_*_params() chuẩn hóa format:
   - RKNN: → object format thống nhất
   - RKLLM: → array format thống nhất
      ↓
5. validate_*_params() kiểm tra tính hợp lệ
      ↓
6. Gọi primitive function với normalized params
      ↓
7. format_*_response() chuẩn hóa response
      ↓
8. Trả về JSON-RPC response thống nhất
```

## Ví dụ thực tế

### RKNN Example - Cả hai cách đều work

```javascript
// Cách 1: Object format (dễ hiểu)
const request1 = {
  "jsonrpc": "2.0",
  "method": "rknn.init",
  "params": {
    "model_path": "/models/qwen2vl.rknn",
    "flag": 0
  },
  "id": 1
};

// Cách 2: Array format (backward compatibility)
const request2 = {
  "jsonrpc": "2.0", 
  "method": "rknn.init",
  "params": ["/models/qwen2vl.rknn", 0],
  "id": 1
};

// Cả hai đều cho kết quả giống nhau
```

### RKLLM Example - Cả hai cách đều work

```javascript
// Cách 1: User-friendly object format  
const request1 = {
  "jsonrpc": "2.0",
  "method": "rkllm.run",
  "params": {
    "prompt": "Explain quantum computing"
  },
  "id": 2
};

// Cách 2: RKLLM native array format
const request2 = {
  "jsonrpc": "2.0",
  "method": "rkllm.run",
  "params": [
    null,
    {
      "input_type": 0,
      "prompt_input": "Explain quantum computing",
      "role": "user", 
      "enable_thinking": false
    },
    {
      "mode": 0,
      "lora_params": null,
      "prompt_cache_params": null,
      "keep_history": 0
    },
    null
  ],
  "id": 2
};

// Cả hai đều cho kết quả giống nhau
```

## Lợi ích cho Developers

### ✅ **Nhất quán**: 
- Không còn confusion giữa RKNN (object) và RKLLM (array)
- Developers có thể chọn format họ thích

### ✅ **Tái sử dụng**:
- JSON helper functions (`extract_string_param`, `extract_int_param`, etc.) dùng chung
- Không duplicate code giữa RKNN và RKLLM

### ✅ **Flexibility**:
- Beginners: Dùng object format dễ hiểu
- Advanced users: Dùng native array format cho control tốt hơn
- Backward compatibility: Code cũ vẫn hoạt động

### ✅ **Validation**:
- Automatic parameter validation cho cả hai APIs
- Clear error messages khi params sai format

## Parameter Mapping Reference

### RKNN Methods
| Method | Object Format | Array Format | Internal Format |
|--------|---------------|--------------|-----------------|
| `rknn.init` | `{model_path, flag}` | `[model_path, flag]` | `{model_path, flag}` |
| `rknn.query` | `{cmd}` | `[cmd]` | `{cmd}` |
| `rknn.run` | `{input_data}` | `[input_data]` | `{input_data}` |

### RKLLM Methods  
| Method | Object Format | Array Format | Internal Format |
|--------|---------------|--------------|-----------------|
| `rkllm.init` | `{param: {...}}` | `[null, {...}, null]` | `[null, {...}, null]` |
| `rkllm.run` | `{prompt, ...}` | `[null, {...}, {...}, null]` | `[null, {...}, {...}, null]` |

## Migration Guide

### Cho existing RKNN clients:
```javascript
// Old way (vẫn work)
{"method": "rknn.init", "params": ["/path/model.rknn", 0]}

// New way (khuyến khích)  
{"method": "rknn.init", "params": {"model_path": "/path/model.rknn", "flag": 0}}
```

### Cho existing RKLLM clients:
```javascript
// Old way (vẫn work)
{"method": "rkllm.run", "params": [null, {...}, {...}, null]}

// New way (dễ dùng hơn)
{"method": "rkllm.run", "params": {"prompt": "Hello world"}}
```

## Testing

Để test JSON Parameter Adapter:

```bash
# Test RKNN với cả object và array format
node tests/test-rknn-formats.js

# Test RKLLM với cả object và array format  
node tests/test-rkllm-formats.js

# Test validation và error handling
node tests/test-param-validation.js
```

---

**Kết luận**: JSON Parameter Adapter giải quyết hoàn toàn hai vấn đề bạn đề cập:
1. ✅ JSON functions được tách ra module chung, tái sử dụng cho cả RKNN và RKLLM
2. ✅ API interface thống nhất - developers không còn bị confusion về parameter format

Server bây giờ có kiến trúc sạch, nhất quán và developer-friendly hơn rất nhiều!