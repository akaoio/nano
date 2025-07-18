#!/bin/bash

# NPU Memory Check Script
# This script verifies NPU memory availability before running tests

echo "🔍 Checking NPU Memory Status..."

# Check if NPU kernel module is loaded
echo "📊 Checking NPU kernel modules..."
if lsmod | grep -q "rknpu"; then
    echo "✅ RKNPU module loaded"
elif lsmod | grep -q "rknn"; then
    echo "✅ RKNN module loaded"
else
    echo "❌ NPU driver not loaded"
    echo "💡 Attempting to load NPU driver..."
    
    # Try to load common NPU drivers
    for driver in rknpu rknn_api galcore; do
        if sudo modprobe $driver 2>/dev/null; then
            echo "✅ Loaded $driver driver"
            break
        fi
    done
    
    # Check again
    if ! lsmod | grep -E "(rknpu|rknn)" > /dev/null; then
        echo "❌ Failed to load NPU driver"
        echo "⚠️  NPU functionality will be limited"
        echo "⚠️  Tests will run in NPU-disabled mode"
        export NPU_DISABLED=1
    fi
fi

# Check NPU device files
echo "📊 Checking NPU device files..."
if ls /dev/rknn* > /dev/null 2>&1; then
    echo "✅ RKNN devices found: $(ls /dev/rknn*)"
elif ls /dev/rknpu* > /dev/null 2>&1; then
    echo "✅ RKNPU devices found: $(ls /dev/rknpu*)"
else
    echo "❌ No NPU device files found"
    echo "⚠️  NPU functionality will be limited"
    export NPU_DISABLED=1
fi

# Check available memory
echo "📊 System Memory:"
free -h

# Check if models exist
echo "📊 Model Files:"
if [ -f "/home/x/Projects/nano/models/qwenvl/model.rkllm" ]; then
    echo "✅ QwenVL model found ($(du -h /home/x/Projects/nano/models/qwenvl/model.rkllm | cut -f1))"
else
    echo "❌ QwenVL model not found"
    export MODEL_MISSING=1
fi

if [ -f "/home/x/Projects/nano/models/lora/model.rkllm" ]; then
    echo "✅ LoRA model found ($(du -h /home/x/Projects/nano/models/lora/model.rkllm | cut -f1))"
else
    echo "❌ LoRA model not found"
    export MODEL_MISSING=1
fi

if [ -f "/home/x/Projects/nano/models/lora/lora.rkllm" ]; then
    echo "✅ LoRA adapter found ($(du -h /home/x/Projects/nano/models/lora/lora.rkllm | cut -f1))"
else
    echo "❌ LoRA adapter not found"
    export MODEL_MISSING=1
fi

echo ""
if [ "$NPU_DISABLED" = "1" ]; then
    echo "⚠️  WARNING: NPU driver not available"
    echo "⚠️  Tests will run in limited mode"
    echo "⚠️  Model loading tests will be skipped"
elif [ "$MODEL_MISSING" = "1" ]; then
    echo "⚠️  WARNING: Some models are missing"
    echo "⚠️  Integration tests may fail"
else
    echo "✅ Pre-flight checks passed"
    echo "🚀 System ready for full testing"
fi
