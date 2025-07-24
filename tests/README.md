# RKLLM Server Test Suite

Comprehensive testing infrastructure for the RKLLM Unix Domain Socket Server, providing complete coverage of all RKLLM functions, server features, and performance characteristics.

## 🧪 Test Suites Overview

### 1. **Basic Functionality Tests** (`test.js`)
**Coverage: Core RKLLM Functions (8/20 functions)**
- ✅ Model lifecycle (init, run, destroy)
- ✅ Parameter creation and constants
- ✅ Streaming functionality
- ✅ Hidden states extraction
- ✅ Performance monitoring
- ✅ Model state management

**Run:** `npm test` or `npm run test:basic`

### 2. **Advanced RKLLM Functions Tests** (`advanced-rkllm-functions.test.js`)
**Coverage: Advanced RKLLM Features (12/20 functions)**
- 🔧 LoRA adapter loading/management
- 🔧 Prompt caching system
- 🔧 KV cache management
- 🔧 Chat template configuration
- 🔧 Function calling tools
- 🔧 Cross-attention parameters
- 🔧 Advanced inference modes
- 🔧 Multimodal input processing

**Run:** `npm run test:advanced`

### 3. **Server Robustness Tests** (`server-robustness.test.js`)
**Coverage: Server Stability & Error Handling**
- 🛡️ Multi-client concurrency
- 🛡️ Malformed JSON handling
- 🛡️ Invalid parameter validation
- 🛡️ Resource cleanup verification
- 🛡️ Connection recovery
- 🛡️ Large input handling
- 🛡️ Memory stability under load

**Run:** `npm run test:robustness`

### 4. **Performance Benchmarks** (`performance-benchmark.test.js`)
**Coverage: Performance Characteristics**
- ⚡ Model initialization timing
- ⚡ Single inference benchmarks
- ⚡ Streaming performance metrics
- ⚡ Throughput testing
- ⚡ Memory usage monitoring

**Run:** `npm run test:performance`

### 5. **Comprehensive Test Suite** (`comprehensive-test-suite.js`)
**Coverage: Complete Testing Orchestra**
- 🎯 Runs all test suites in sequence
- 📊 Detailed coverage analysis
- 🚀 Production readiness assessment
- 📈 Performance summary report

**Run:** `npm run test:comprehensive` or `npm run test:all`

## 🚀 Quick Start

```bash
# Run basic tests (recommended for development)
npm test

# Run comprehensive tests (recommended for production)
npm run test:all

# Run specific test suite
npm run test:advanced
npm run test:robustness
npm run test:performance
```

## 📊 Test Coverage Breakdown

| Test Suite | RKLLM Functions | Server Features | Production Critical |
|------------|-----------------|-----------------|-------------------|
| Basic Tests | 8/20 (40%) | 2/10 (20%) | ✅ Essential |
| Advanced Tests | 12/20 (60%) | 1/10 (10%) | 🔶 Important |
| Robustness Tests | 0/20 (0%) | 6/10 (60%) | ✅ Critical |
| Performance Tests | 0/20 (0%) | 2/10 (20%) | 🔶 Important |
| **Combined Coverage** | **20/20 (100%)** | **10/10 (100%)** | **🎯 Complete** |

## 🎯 Production Readiness Validation

The comprehensive test suite evaluates production readiness based on:

- **Critical Suites**: Basic + Robustness tests must pass
- **Test Success Rate**: >90% individual test pass rate
- **Coverage**: >80% function and feature coverage
- **Performance**: Response times within acceptable thresholds

### Readiness Levels:
- **🚀 READY (90-100%)**: Deploy to production
- **🔶 MOSTLY READY (70-89%)**: Address failing tests first
- **⚠️ NOT READY (50-69%)**: Significant improvements needed
- **❌ INSUFFICIENT (<50%)**: Major rework required

## 🔧 Advanced Usage

### Command Line Options

```bash
# Skip specific test suites
npm run test:comprehensive -- --skip-advanced
npm run test:comprehensive -- --skip-performance

# Continue on failure (don't stop at first failing suite)
npm run test:comprehensive -- --continue-on-failure

# Show help
npm run test:comprehensive -- --help
```

### Custom Test Configuration

Tests use model files from:
- **Standard Model**: `/home/x/Projects/nano/models/qwen3/model.rkllm`
- **LoRA Model**: `/home/x/Projects/nano/models/lora/model.rkllm`
- **LoRA Adapter**: `/home/x/Projects/nano/models/lora/lora.rkllm`

Update paths in `tests/lib/test-helpers.js` if needed.

## 🧪 Test Architecture

### Core Libraries
- **`tests/lib/server-manager.js`**: Server lifecycle management
- **`tests/lib/test-client.js`**: JSON-RPC client for testing
- **`tests/lib/test-helpers.js`**: Common utilities and data structures

### Test Pattern
```javascript
class MyTests {
  async runTest(name, testFunction) {
    // Standardized test execution with error handling
  }
  
  async setup() {
    // Server startup and initialization
  }
  
  async teardown() {
    // Cleanup and resource management
  }
}
```

## 📈 Performance Thresholds

| Metric | Acceptable | Good | Excellent |
|--------|------------|------|-----------|
| Model Init | <10s | <5s | <3s |
| Single Inference | <5s | <2s | <1s |
| Streaming Rate | >1 tok/s | >5 tok/s | >10 tok/s |
| Throughput | >1 req/s | >5 req/s | >10 req/s |
| Memory Stability | <50MB variance | <20MB | <10MB |

## 🛠️ Development Guidelines

### Adding New Tests

1. **Create test file**: `tests/my-new-tests.test.js`
2. **Follow pattern**: Use existing test classes as templates
3. **Add to package.json**: Include script for easy running
4. **Update comprehensive**: Include in comprehensive test suite
5. **Document**: Update this README with coverage info

### Test Best Practices

- ✅ Always test both success and failure cases
- ✅ Include proper cleanup in teardown methods
- ✅ Use descriptive test names and error messages
- ✅ Test edge cases and boundary conditions
- ✅ Verify resource cleanup and memory stability
- ✅ Include performance measurements where relevant

## 🔍 Troubleshooting

### Common Issues

**Server startup timeout**
```bash
# Check if server executable exists
ls -la build/server

# Check server logs
timeout 5 ./build/server
```

**Model file not found**
```bash
# Verify model paths exist
ls -la models/qwen3/model.rkllm
ls -la models/lora/
```

**Connection refused**
```bash
# Ensure no other server is running
pkill -f server
lsof -i :8080  # Check if port is in use
```

**Memory issues**
```bash
# Monitor memory during tests
top -p $(pgrep server)
```

## 📊 Test Results Interpretation

### Sample Output
```
🎉 COMPREHENSIVE TEST SUITE REPORT
📊 OVERALL RESULTS:
   Test Suites: 4/4 passed
   Total Tests: 28/30 passed
   Total Duration: 45.2s

🚀 Production Readiness: READY (94.5/100)
✅ Recommended: DEPLOY TO PRODUCTION
```

### Key Metrics
- **Suite Pass Rate**: Percentage of test suites passing
- **Individual Test Rate**: Percentage of individual tests passing
- **Coverage Analysis**: Function and feature coverage percentages
- **Performance Summary**: Key performance metrics from benchmarks
- **Production Score**: Overall readiness score (0-100)

## 🎯 Future Enhancements

- [ ] Security testing suite (input validation, buffer overflows)
- [ ] Load testing with sustained high traffic
- [ ] Integration tests with real client applications
- [ ] Regression testing against previous versions
- [ ] Automated CI/CD integration
- [ ] Docker container testing
- [ ] Cross-platform compatibility tests

---

**For questions or contributions, please refer to the main project documentation.**