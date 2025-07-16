# MIGRATION PLAN - Clean Slate Refactor

## PHASE 3: INCREMENTAL MIGRATION

### Step 3.1: Create New Structure
```bash
# Create new source structure
mkdir -p src_new/io/core/{queue,worker_pool,io}
mkdir -p src_new/io/mapping/{handle_pool,rkllm_proxy}
mkdir -p src_new/nano/core/nano
mkdir -p src_new/nano/validation/model_checker
mkdir -p src_new/nano/system/{system_info,resource_mgr}
mkdir -p src_new/nano/transport/{mcp_base,udp_transport,tcp_transport,http_transport,ws_transport,stdio_transport}
mkdir -p src_new/common/{json_utils,memory_utils,string_utils}

# Create new test structure
mkdir -p tests_new/io/{test_queue,test_worker_pool,test_handle_pool,test_io}
mkdir -p tests_new/nano/{test_validation,test_system,test_transport,test_nano}
mkdir -p tests_new/integration/{test_qwenvl,test_lora,test_mcp}
```

### Step 3.2: Function Mapping Matrix
| Current File | Current Function | New Location | New Function Name |
|-------------|------------------|--------------|-------------------|
| src/io/io.c | io_init() | src_new/io/core/io/io.c | io_init() |
| src/io/queue.c | queue_push() | src_new/io/core/queue/queue.c | queue_push() |
| src/io/handle_pool.c | handle_pool_create() | src_new/io/mapping/handle_pool/handle_pool.c | handle_pool_create() |
| src/io/operations.c | method_init() | src_new/io/mapping/rkllm_proxy/rkllm_proxy.c | rkllm_proxy_init() |
| src/io/system_info.c | system_force_gc() | src_new/nano/system/system_info/system_info.c | system_force_gc() |
| src/io/model_version.c | extract_model_version() | src_new/nano/validation/model_checker/model_checker.c | model_check_version() |

### Step 3.3: Dependency Migration Order
1. **Common utilities** (no dependencies)
2. **IO core** (depends on common)
3. **IO mapping** (depends on io core + rkllm)
4. **Nano system** (depends on common)
5. **Nano validation** (depends on common + nano system)
6. **Nano transport** (depends on all above)
7. **Nano core** (depends on all above)
8. **Main** (depends on nano core)

### Step 3.4: Test Migration Order
1. **Unit tests** for each new component
2. **Integration tests** for each module
3. **End-to-end tests** for full system

## PHASE 4: VALIDATION & VERIFICATION

### Step 4.1: Function Parity Check
```bash
# Compare function signatures
./scripts/check_function_parity.sh src/ src_new/

# Compare test coverage
./scripts/check_test_coverage.sh tests/ tests_new/
```

### Step 4.2: API Compatibility Check
```bash
# Generate API documentation for old system
./scripts/generate_api_docs.sh src/ > old_api.md

# Generate API documentation for new system
./scripts/generate_api_docs.sh src_new/ > new_api.md

# Compare APIs
diff old_api.md new_api.md
```

### Step 4.3: Performance Validation
```bash
# Benchmark old system
./scripts/benchmark_old.sh > old_performance.txt

# Benchmark new system
./scripts/benchmark_new.sh > new_performance.txt

# Compare performance
./scripts/compare_performance.sh old_performance.txt new_performance.txt
```

## PHASE 5: CUTOVER

### Step 5.1: Atomic Replacement
```bash
# Only when new system is 100% validated
mv src/ src_old_backup/
mv src_new/ src/
mv tests/ tests_old_backup/
mv tests_new/ tests/
mv Makefile_new Makefile
```

### Step 5.2: Final Verification
```bash
# Full system test
make clean && make test

# Integration test with real models
./test_qwenvl_integration.sh
./test_lora_integration.sh
```

### Step 5.3: Cleanup
```bash
# Only after 100% confirmation
rm -rf src_old_backup/
rm -rf tests_old_backup/
rm -rf migration_temp/
```

## SAFETY MECHANISMS

### 1. Rollback Plan
```bash
# If anything goes wrong
git checkout backup-before-refactor
# System is immediately restored
```

### 2. Parallel Testing
```bash
# During migration, both systems run in parallel
make test_old  # Test old system
make test_new  # Test new system
```

### 3. Incremental Validation
- Each component migrated must pass unit tests
- Each module migrated must pass integration tests
- Full system migrated must pass end-to-end tests

### 4. Zero Downtime Migration
- New system built completely before replacing old
- No partial states
- Atomic cutover

## SUCCESS CRITERIA

### Technical Criteria
- [ ] All existing functions preserved
- [ ] All existing tests pass
- [ ] No performance regression
- [ ] No memory leaks
- [ ] Clean compilation (no warnings)

### Structural Criteria
- [ ] No file > 100 lines
- [ ] Clear module boundaries
- [ ] No circular dependencies
- [ ] Consistent naming conventions
- [ ] Complete documentation

### Quality Criteria
- [ ] Code coverage >= current level
- [ ] Static analysis passes
- [ ] Memory sanitizer passes
- [ ] Integration tests pass
- [ ] Real model tests pass

## RISK MITIGATION

### High Risk Areas
1. **Handle management** - Complex pointer handling
2. **Thread safety** - Worker pool synchronization
3. **Memory management** - Potential leaks
4. **API compatibility** - External interfaces

### Mitigation Strategies
1. **Extensive testing** at each step
2. **Peer review** for high-risk components
3. **Gradual rollout** with rollback capability
4. **Comprehensive monitoring** during migration
