# LoRa and Logits Usage Guide

## Overview

This guide covers the proper usage of LoRa (Low-Rank Adaptation) functionality and logits extraction with the RKLLM server.

## LoRa Functionality

### Required Files Structure
```
models/lora/
├── model.rkllm      # Base model file
└── lora.rkllm       # LoRa adapter file
```

### Complete LoRa Workflow

#### 1. Initialize with Base Model
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "rkllm.init",
  "params": [
    null,
    {
      "model_path": "/home/x/Projects/nano/models/lora/model.rkllm",
      "max_context_len": 512,
      "max_new_tokens": 256,
      "top_k": 1,
      "top_p": 0.9,
      "temperature": 0.8,
      "repeat_penalty": 1.1,
      "frequency_penalty": 0.0,
      "presence_penalty": 0.0
    },
    null
  ]
}
```

#### 2. Load LoRa Adapter
```json
{
  "jsonrpc": "2.0",
  "id": 2,
  "method": "rkllm.load_lora",
  "params": [
    null,
    {
      "lora_adapter_path": "/home/x/Projects/nano/models/lora/lora.rkllm",
      "lora_adapter_name": "main_adapter",
      "scale": 1.0
    }
  ]
}
```

#### 3. Run Inference with LoRa
```json
{
  "jsonrpc": "2.0",
  "id": 3,
  "method": "rkllm.run",
  "params": [
    null,
    {
      "role": "user",
      "input_type": 0,
      "prompt_input": "Hello, how are you today?"
    },
    {
      "mode": 0,
      "lora_params": {
        "lora_adapter_name": "main_adapter"
      },
      "keep_history": 0
    },
    null
  ]
}
```

## Logits Functionality

### What are Logits?
Logits are the raw, unnormalized output values from the language model before applying softmax. They represent the model's confidence in each possible next token.

### Extracting Logits
```json
{
  "jsonrpc": "2.0",
  "id": 4,
  "method": "rkllm.run",
  "params": [
    null,
    {
      "role": "user",
      "input_type": 0,
      "prompt_input": "The capital of France is"
    },
    {
      "mode": 2,
      "keep_history": 0
    },
    null
  ]
}
```

### Expected Logits Response
```json
{
  "jsonrpc": "2.0",
  "id": 4,
  "result": {
    "text": "Paris",
    "logits": [0.123, -0.456, 2.789, ...],
    "vocab_size": 32000,
    "token_count": 1
  }
}
```

## Advanced Usage

### Combining LoRa with Logits
```json
{
  "jsonrpc": "2.0",
  "id": 5,
  "method": "rkllm.run",
  "params": [
    null,
    {
      "role": "user",
      "input_type": 0,
      "prompt_input": "Explain quantum computing"
    },
    {
      "mode": 2,
      "lora_params": {
        "lora_adapter_name": "main_adapter"
      },
      "keep_history": 0
    },
    null
  ]
}
```

## Troubleshooting

### Common LoRa Issues

1. **"Model not initialized" Error**
   - Solution: Always call `rkllm.init` with the base model first
   - Verify model path exists: `/home/x/Projects/nano/models/lora/model.rkllm`

2. **"LoRa adapter not found" Error**
   - Solution: Verify adapter path exists: `/home/x/Projects/nano/models/lora/lora.rkllm`
   - Check that `lora_adapter_name` matches exactly

3. **Slow Initialization**
   - Large models can take 30+ seconds to initialize
   - Monitor server logs for progress
   - Ensure sufficient RAM available

### Common Logits Issues

1. **Logits Mode Timeout/Hang**
   - Some models may not support logits extraction
   - Try with mode 0 (normal inference) first to verify model works
   - Check model compatibility with RKLLM version

2. **Empty Logits Array**
   - Verify the model supports logits output
   - Check if special tokens are affecting output

### Testing Connection
```bash
# Test if server is running
nc -U /tmp/rkllm.sock

# Send test request
echo '{"jsonrpc":"2.0","id":1,"method":"rkllm.get_constants"}' | nc -U /tmp/rkllm.sock
```

### Validation Steps

1. **Check File Existence**
   ```bash
   ls -la /home/x/Projects/nano/models/lora/
   # Should show both model.rkllm and lora.rkllm
   ```

2. **Test Basic Functionality**
   ```bash
   # Run the validation script
   node /home/x/Projects/nano/sandbox/validate_lora_and_logits.js
   ```

3. **Monitor Server Logs**
   ```bash
   # Start server with verbose logging
   ./build/rkllm_uds_server
   ```

## Performance Notes

- **LoRa Overhead**: LoRa adapters add ~10-20% computational overhead
- **Logits Memory**: Logits arrays can be large (vocab_size * sizeof(float))
- **Model Loading**: Initial model load can take 30-60 seconds for large models
- **Concurrent Requests**: Server handles one model at a time; multiple requests queue

## API Reference

### Mode Values
- `0`: Normal text generation
- `1`: Reserved/Unused
- `2`: Extract logits (may timeout on incompatible models)

### Input Types
- `0`: Text prompt
- `1`: Token IDs
- `2`: Multimodal input

### LoRa Scale Values
- `0.0`: Disable LoRa completely
- `1.0`: Full LoRa strength (recommended)
- `>1.0`: Amplified LoRa effect (experimental)

## Example Scripts

See the following files for working examples:
- `/home/x/Projects/nano/sandbox/test_lora_and_logits.js`
- `/home/x/Projects/nano/sandbox/validate_lora_and_logits.js`
- `/home/x/Projects/nano/test-comprehensive.js`