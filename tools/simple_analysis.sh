#!/bin/bash
# Simple working analysis script

echo "🔍 Running LLVM Analysis..."

# Create reports directories
mkdir -p reports/{ast,ir,quality,architecture,dead_code,duplicates}

# 1. Generate LLVM IR for transport files
echo "⚡ Generating LLVM IR..."
for transport in tcp udp http stdio; do
    if [ -f "src/lib/transport/${transport}.c" ]; then
        echo "  Processing ${transport} transport..."
        clang -emit-llvm -S "src/lib/transport/${transport}.c" \
            -I src/include -I src/external/rkllm -I src/common \
            -o "reports/ir/${transport}_transport.ll" 2>/dev/null || true
        if [ -f "reports/ir/${transport}_transport.ll" ]; then
            echo "    Generated $(wc -l < reports/ir/${transport}_transport.ll) lines of IR"
        fi
    fi
done

# 2. Run static analysis
echo "🔬 Running static analysis..."
clang-tidy src/lib/transport/tcp.c \
    -checks=bugprone-*,readability-function-cognitive-complexity,performance-* \
    --header-filter=src/include/ > reports/quality/analysis_results.txt 2>/dev/null || true

echo "  Found $(wc -l < reports/quality/analysis_results.txt) analysis findings"

# 3. Generate AST for key files
echo "🌳 Generating AST dumps..."
clang -Xclang -ast-dump -fsyntax-only src/lib/transport/tcp.c \
    -I src/include -I src/external/rkllm -I src/common \
    > reports/ast/tcp_transport_ast.txt 2>/dev/null || true

if [ -f "reports/ast/tcp_transport_ast.txt" ]; then
    echo "  Generated $(wc -l < reports/ast/tcp_transport_ast.txt) lines of AST"
fi

# 4. Create summary
echo "📊 Creating analysis summary..."
cat > reports/analysis_summary.txt << SUMMARY
RKLLM MCP Server Analysis Summary
================================
Generated: $(date)

LLVM IR Files Generated:
$(ls -la reports/ir/*.ll 2>/dev/null | awk '{print "- " $9 ": " $5 " bytes"}' || echo "- No IR files found")

Static Analysis Results:
- File: reports/quality/analysis_results.txt
- Issues found: $(wc -l < reports/quality/analysis_results.txt 2>/dev/null || echo "0")

AST Analysis:
$(ls -la reports/ast/*.txt 2>/dev/null | awk '{print "- " $9 ": " $5 " bytes"}' || echo "- No AST files found")

Key Findings:
$(head -5 reports/quality/analysis_results.txt 2>/dev/null | grep -E "warning|error" | head -3 || echo "- See detailed reports for findings")

SUMMARY

echo "✅ Analysis complete!"
echo "📋 Summary: reports/analysis_summary.txt"
echo "📂 Detailed reports in: reports/"
