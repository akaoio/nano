<!-- filepath: /home/x/Projects/nano/docs/LLVM_ANALYSIS_GUIDE.md -->
# LLVM/Clang Code Analysis Guide
## Comprehensive Architecture Analysis for RKLLM MCP Server

This guide demonstrates how to use LLVM/Clang tools to deeply analyze the RKLLM MCP Server codebase, visualize architecture, hunt down duplicated code, dead code, and understand the system structure.

**Important**: The `reports/` directory is auto-generated and git-ignored. All analysis outputs go there and are created by running `make analyze`.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Architecture Analysis](#architecture-analysis)
3. [AST (Abstract Syntax Tree) Analysis](#ast-analysis)
4. [LLVM IR Analysis](#llvm-ir-analysis)
5. [Code Quality Analysis](#code-quality-analysis)
6. [Dead Code Detection](#dead-code-detection)
7. [Duplicate Code Detection](#duplicate-code-detection)
8. [Performance Analysis](#performance-analysis)

---

## Quick Start

The easiest way to run comprehensive analysis:

```bash
# Build the project first
cd build
cmake .. && make

# Run comprehensive analysis (auto-generates everything in reports/)
make analyze

# View the master report
cat ../reports/COMPREHENSIVE_ARCHITECTURE_ANALYSIS.md

# Browse all auto-generated reports
ls -la ../reports/
```

This single command generates 30+ analysis files across multiple categories, all in the auto-generated `reports/` directory.

---

## Architecture Analysis

### Auto-Generated Architecture Reports

All architecture analysis is auto-generated in `reports/architecture/`:

```bash
# Run analysis to generate architecture reports
make analyze

# View auto-generated architecture insights
cat reports/architecture/function_dependencies.md
cat reports/architecture/complexity_analysis.txt
```

### Transport Layer Analysis

Auto-generated transport analysis in `reports/transport_analysis/`:

```bash
# After running make analyze, view transport reports
cat reports/transport_analysis/transport_architecture.md
cat reports/transport_analysis/pattern_duplication.md
```

### Call Graph Analysis

Auto-generated call graphs in `reports/call_graphs/`:

```bash
# View auto-generated call graph analysis
cat reports/call_graphs/tcp_analysis.md
cat reports/call_graphs/http_analysis.md
```

---

## AST (Abstract Syntax Tree) Analysis

### Auto-Generated AST Reports

AST dumps are auto-generated in `reports/ast/`:

```bash
# Run analysis to generate AST dumps
make analyze

# View auto-generated AST for key components
head -50 reports/ast/tcp_ast.txt
head -50 reports/ast/rkllm_proxy_ast.txt
```

### Custom AST Analysis Tools

Create analysis tools that output to `reports/` (never commit files there):

```python
# tools/custom_ast_analyzer.py
import clang.cindex
import json
import os

def analyze_transport_patterns():
    """Analyze transport patterns and output to reports/"""
    
    # Ensure reports directory exists (auto-created by make analyze)
    os.makedirs('reports/custom_analysis', exist_ok=True)
    
    # Initialize libclang
    clang.cindex.conf.set_library_path('/usr/lib/llvm-18/lib')
    index = clang.cindex.Index.create()
    
    transport_files = [
        'src/lib/transport/stdio.c',
        'src/lib/transport/tcp.c', 
        'src/lib/transport/udp.c',
        'src/lib/transport/http.c'
    ]
    
    patterns = {}
    
    for transport_file in transport_files:
        if os.path.exists(transport_file):
            tu = index.parse(transport_file, args=['-I./src/include'])
            patterns[transport_file] = analyze_transport_implementation(tu)
    
    # Output to auto-generated reports directory
    with open('reports/custom_analysis/transport_patterns.json', 'w') as f:
        json.dump(patterns, f, indent=2)

def analyze_transport_implementation(tu):
    """Extract function patterns from transport implementation"""
    functions = []
    
    for node in tu.cursor.walk_preorder():
        if node.kind == clang.cindex.CursorKind.FUNCTION_DECL:
            functions.append({
                'name': node.spelling,
                'return_type': node.result_type.spelling,
                'parameters': [param.type.spelling for param in node.get_arguments()]
            })
    
    return functions

if __name__ == "__main__":
    analyze_transport_patterns()
```

---

## LLVM IR Analysis

### Auto-Generated IR Files

LLVM IR files are auto-generated in `reports/ir/`:

```bash
# Run analysis to generate IR files
make analyze

# View auto-generated IR files
ls -la reports/ir/
cat reports/ir/tcp_transport.ll | head -20
```

### IR Analysis Examples

```bash
# Control flow analysis (outputs to reports/)
opt -analyze -print-cfg reports/ir/rkllm_proxy_core.ll > reports/custom_analysis/control_flow.txt

# Data flow analysis (outputs to reports/)
opt -analyze -print-alias-sets reports/ir/*.ll > reports/custom_analysis/data_flow.txt

# Memory access pattern analysis (outputs to reports/)
opt -analyze -print-memoryssa reports/ir/*.ll > reports/custom_analysis/memory_patterns.txt
```

---

## Code Quality Analysis

### Auto-Generated Quality Reports

Static analysis reports are auto-generated in `reports/quality/`:

```bash
# Run analysis to generate quality reports
make analyze

# View auto-generated quality analysis
cat reports/quality/comprehensive_static_analysis.txt
cat reports/quality/issues_by_category.md
```

### Additional Quality Tools

```bash
# Run additional analysis (outputs to reports/)
cppcheck src/lib/ --xml > reports/custom_analysis/cppcheck_results.xml

# Complexity analysis (outputs to reports/)
lizard src/lib/**/*.c --csv > reports/custom_analysis/complexity_detailed.csv
```

---

## Dead Code Detection

### Auto-Generated Dead Code Reports

Dead code analysis is auto-generated in `reports/dead_code/`:

```bash
# Run analysis to generate dead code reports
make analyze

# View auto-generated dead code analysis
cat reports/dead_code/comprehensive_dead_code_analysis.md
cat reports/dead_code/static_functions.txt
```

### Custom Dead Code Analysis

```python
# tools/advanced_dead_code_detector.py
import os
import re

def advanced_dead_code_analysis():
    """Advanced dead code detection outputting to reports/"""
    
    # Ensure reports directory exists
    os.makedirs('reports/dead_code', exist_ok=True)
    
    # Read auto-generated function data
    with open('reports/dead_code/all_function_definitions.txt', 'r') as f:
        definitions = f.read()
    
    with open('reports/dead_code/all_function_calls.txt', 'r') as f:
        calls = f.read()
    
    # Advanced analysis logic here...
    
    # Output to reports/
    with open('reports/dead_code/advanced_analysis.md', 'w') as f:
        f.write("# Advanced Dead Code Analysis\n")
        f.write("Auto-generated by advanced_dead_code_detector.py\n\n")
        # ... analysis results

if __name__ == "__main__":
    advanced_dead_code_analysis()
```

---

## Duplicate Code Detection

### Auto-Generated Duplicate Reports

Duplicate code analysis is auto-generated in `reports/duplicates/`:

```bash
# Run analysis to generate duplicate reports
make analyze

# View auto-generated duplicate analysis
cat reports/duplicates/pattern_analysis.txt
cat reports/duplicates/repeated_blocks.txt
```

### Custom Duplicate Detection

```python
# tools/semantic_duplicate_detector.py
import clang.cindex
import os
import hashlib

def find_semantic_duplicates():
    """Find semantically similar code blocks, output to reports/"""
    
    # Ensure reports directory exists
    os.makedirs('reports/duplicates', exist_ok=True)
    
    clang.cindex.conf.set_library_path('/usr/lib/llvm-18/lib')
    index = clang.cindex.Index.create()
    
    source_files = [
        'src/lib/transport/stdio.c',
        'src/lib/transport/tcp.c',
        'src/lib/transport/udp.c',
        'src/lib/transport/http.c'
    ]
    
    duplicates = []
    function_hashes = {}
    
    for source_file in source_files:
        if os.path.exists(source_file):
            tu = index.parse(source_file, args=['-I./src/include'])
            
            for node in tu.cursor.walk_preorder():
                if node.kind == clang.cindex.CursorKind.FUNCTION_DECL:
                    func_signature = extract_function_signature(node)
                    func_hash = hashlib.md5(func_signature.encode()).hexdigest()
                    
                    if func_hash in function_hashes:
                        duplicates.append({
                            'original': function_hashes[func_hash],
                            'duplicate': f"{source_file}::{node.spelling}",
                            'signature': func_signature
                        })
                    else:
                        function_hashes[func_hash] = f"{source_file}::{node.spelling}"
    
    # Output to reports/
    with open('reports/duplicates/semantic_duplicates.md', 'w') as f:
        f.write("# Semantic Duplicate Analysis\n")
        f.write("Auto-generated by semantic_duplicate_detector.py\n\n")
        
        if duplicates:
            f.write(f"Found {len(duplicates)} potential duplicates:\n\n")
            for dup in duplicates:
                f.write(f"## Potential Duplicate\n")
                f.write(f"- **Original**: {dup['original']}\n")
                f.write(f"- **Duplicate**: {dup['duplicate']}\n")
                f.write(f"- **Signature**: `{dup['signature']}`\n\n")
        else:
            f.write("✅ No semantic duplicates found!\n")

def extract_function_signature(node):
    """Extract normalized function signature"""
    params = []
    for param in node.get_arguments():
        params.append(param.type.spelling)
    
    return f"{node.result_type.spelling} {len(params)} {' '.join(sorted(params))}"

if __name__ == "__main__":
    find_semantic_duplicates()
```

---

## Performance Analysis

### Auto-Generated Performance Reports

Performance metrics are auto-generated in `reports/metrics/`:

```bash
# Run analysis to generate performance reports
make analyze

# View auto-generated performance metrics
cat reports/metrics/comprehensive_metrics.md
```

### Custom Performance Tools

```bash
# Profile-guided analysis (outputs to reports/)
clang -fprofile-generate src/main.c -o build/mcp_server_profile
./build/mcp_server_profile # Run some tests
llvm-profdata merge -output=reports/performance/profile.profdata default.profraw
llvm-profdata show reports/performance/profile.profdata > reports/performance/hot_functions.txt
```

---

## Integration with Build System

### CMake Integration

The analysis is integrated into the build system via `make analyze`:

```cmake
# In CMakeLists.txt - already implemented
add_custom_target(analyze
    COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/analyze.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running comprehensive analysis (auto-generates reports/)..."
)
```

### Analysis Script Structure

The `scripts/analyze.sh` script auto-generates all reports:

```bash
# Key points from scripts/analyze.sh:
# 1. Clears reports/ directory first
# 2. Auto-generates all analysis files
# 3. Creates comprehensive summary
# 4. Validates all outputs exist
```

---

## Viewing Results

### Master Report

Always start with the auto-generated master report:

```bash
# View the comprehensive analysis summary
cat reports/COMPREHENSIVE_ARCHITECTURE_ANALYSIS.md
```

### Category-Specific Reports

```bash
# View specific analysis categories
ls reports/transport_analysis/    # Transport layer insights
ls reports/security_analysis/     # Security assessment  
ls reports/streaming_analysis/    # Streaming architecture
ls reports/call_graphs/          # Function interactions
ls reports/dependency_analysis/   # Module dependencies
```

### Quick Statistics

```bash
# Get quick stats on auto-generated reports
echo "Files generated: $(find reports/ -type f | wc -l)"
echo "Total analysis data: $(du -sh reports/)"
echo "Key insights: $(find reports/ -name "*.md" | wc -l) reports"
```

---

## Custom Analysis Development

### Guidelines for Custom Tools

When creating custom analysis tools:

1. **Always output to `reports/`** - Never commit files there
2. **Use `os.makedirs('reports/subdir', exist_ok=True)`** for safety
3. **Generate markdown reports** for readability
4. **Include timestamps** in generated files
5. **Handle missing source files gracefully**

### Example Custom Tool Template

```python
# tools/custom_analyzer_template.py
import os
from datetime import datetime

def custom_analysis():
    """Template for custom analysis tools"""
    
    # Ensure output directory exists (but don't commit)
    os.makedirs('reports/custom_analysis', exist_ok=True)
    
    # Perform your analysis
    results = perform_analysis()
    
    # Generate report in reports/ (auto-generated, git-ignored)
    with open('reports/custom_analysis/my_analysis.md', 'w') as f:
        f.write(f"# Custom Analysis Report\n")
        f.write(f"Generated: {datetime.now()}\n\n")
        f.write("## Results\n")
        # ... your results here

def perform_analysis():
    """Your analysis logic here"""
    return {}

if __name__ == "__main__":
    custom_analysis()
```

---

## Best Practices

### 1. Always Use Auto-Generation
- Never manually create files in `reports/`
- Use `make analyze` for comprehensive analysis
- Create custom tools that output to `reports/`

### 2. Report Organization
- Use descriptive subdirectories in `reports/`
- Generate markdown for human readability
- Include timestamps and metadata

### 3. Integration
- Add custom tools to `scripts/analyze.sh` if they're generally useful
- Use CMake targets for specialized analysis
- Document custom tools in this guide

### 4. Validation
- Always check if source files exist before analyzing
- Handle errors gracefully in analysis scripts
- Validate that expected outputs are generated

This guide ensures all analysis outputs are properly auto-generated and git-ignored while providing comprehensive insights into the RKLLM MCP Server architecture.