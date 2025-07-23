# LoRa and Logits Usage Guide

**Date:** 2025-07-23 23:55  
**Status:** COMPREHENSIVE GUIDE

## LoRa (Low-Rank Adaptation) Usage

### Required Files
- **Base Model**: `/home/x/Projects/nano/models/lora/model.rkllm` - The original model
- **LoRa Adapter**: `/home/x/Projects/nano/models/lora/lora.rkllm` - The adaptation weights

### Proper LoRa Workflow

1. **Initialize Model** with base model:
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "rkllm.init",
  "params": [{
    "model_path": "/home/x/Projects/nano/models/lora/model.rkllm",
    "max_context_len": 512,
    "max_new_tokens": 256
  }]
}
```

2. **Load LoRa Adapter**:
```json
{
  "jsonrpc": "2.0",
  "id": 2,
  "method": "rkllm.load_lora",
  "params": [{
    "lora_adapter_path": "/home/x/Projects/nano/models/lora/lora.rkllm",
    "lora_adapter_name": "test_adapter",
    "scale": 1.0
  }]
}
```

3. **Run Inference** with LoRa parameters:
```json
{
  "jsonrpc": "2.0",
  "id": 3,
  "method": "rkllm.run",
  "params": [
    null,
    {
      "role": "user",
      "enable_thinking": false,
      "input_type": 0,
      "prompt_input": "Hello, how are you?"
    },
    {
      "mode": 0,
      "lora_params": {
        "lora_adapter_name": "test_adapter"
      },
      "keep_history": 1
    },
    null
  ]
}
```

## Logits Mode Issues and Solutions

### Current Problem
The logits extraction mode (mode 2) is causing the server to hang indefinitely. This indicates:

1. **Model Compatibility**: Not all RKLLM models support logits extraction
2. **Library Version**: The RKLLM library version may have logits mode issues
3. **Implementation**: The mode may require specific setup or parameters

### Debugging Steps

1. **Test with Different Models**:
   - Try logits mode with the standard model: `/home/x/Projects/nano/models/qwen3/model.rkllm`
   - Check if the issue is model-specific

2. **Implement Timeout Protection**:
   - Add timeout mechanism to prevent infinite hanging
   - Gracefully handle logits mode failures

3. **Verify Library Support**:
   - Check RKLLM library documentation for logits mode requirements
   - Verify if specific model types support logits extraction

### Temporary Workaround

For now, skip logits mode testing until we can:
1. Verify model compatibility
2. Implement proper timeout handling
3. Get clarification on RKLLM logits mode requirements

## Testing Recommendations

### LoRa Testing
1. **Basic LoRa Workflow**: Test the 3-step process above
2. **Adapter Switching**: Load different adapters and verify behavior
3. **Scale Testing**: Test different scale values (0.5, 1.0, 2.0)

### Logits Mode Testing
1. **Model Compatibility**: Test with different model files
2. **Timeout Implementation**: Add 30-second timeout for logits mode
3. **Error Handling**: Graceful fallback when logits mode fails

## Implementation Status

### LoRa - ✅ READY
- [x] rkllm.load_lora implemented
- [x] LoRa parameter conversion working
- [x] Files available for testing
- [x] Workflow documented

### Logits Mode - ⚠️ BLOCKED
- [x] Mode 2 parameter conversion working
- [x] JSON conversion implemented
- [ ] Hanging issue resolved
- [ ] Timeout protection added
- [ ] Model compatibility verified

## Next Actions

1. **Implement LoRa Test**: Create focused test for LoRa workflow
2. **Add Logits Timeout**: Implement timeout protection for logits mode
3. **Model Testing**: Test logits mode with different models
4. **Documentation**: Update results based on testing outcomes