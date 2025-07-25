# Development Instructions

## 🔥 CRITICAL READING ORDER
1. **DESIGN.md** - Complete architecture (MANDATORY)
2. **docs/notes/** - All status files (MANDATORY)  
3. **This file** - Development workflow

## 🎯 PROJECT COMPLETION CRITERIA
- ✅ Server runs without errors
- ✅ Complete test suite passes (`npm test`)
- ✅ Real functionality demonstrated per DESIGN.md

## 📋 DEVELOPMENT WORKFLOW

### Quality Standards
- ❌ **NEVER**: Fake code, TODOs, hardcoded paths
- ✅ **ALWAYS**: Real production code, proper testing

### File Organization
- **Tests**: `tests/` (official) and `sandbox/` (temporary)
- **Documentation**: `docs/notes/YYYYMMDD_HH-MM_<topic>.md`
- **Issues**: `docs/issues/YYYYMMDD_HH-MM_<issue>.md`

## 🔗 DEPENDENCIES

### Required Libraries
- **RKLLM**: https://github.com/airockchip/rknn-llm
- **RKNN**: https://github.com/airockchip/rknn-toolkit2  
- **JSON-C**: For JSON parsing

### Test Models (Use Environment Variables)
- **Standard**: `/home/x/Projects/nano/models/qwen3/model.rkllm`
- **LoRa Base**: `/home/x/Projects/nano/models/lora/model.rkllm`  
- **LoRa Adapter**: `/home/x/Projects/nano/models/lora/lora.rkllm`

## 🧪 TESTING GUIDELINES

### Server Testing (IMPORTANT)
- ⚠️ **NEVER**: Start server directly (causes hanging)
- ✅ **CORRECT**: Start server from within test scripts
- ✅ **METHOD**: Concurrent server + test execution

### Test Requirements
- **Automated**: Programmatic verification
- **Measurable**: Clear pass/fail criteria  
- **Technical**: Real input/output validation

## 🚨 ISSUE HANDLING
1. **Document immediately**: Create `docs/issues/YYYYMMDD_HH-MM_<issue>.md`
2. **Include details**: Problem, reproduction steps, attempts
3. **Reference back**: Link to related documentation

## 📝 NOTES TEMPLATE
```markdown
# YYYYMMDD_HH-MM_<Task>

## Status
- Current work
- Progress made

## Plan  
- [ ] Next steps
- [ ] Dependencies

## Results
- Test outcomes
- Key findings

## Next Actions
- Immediate tasks
- Blockers
```

## 🎯 SUCCESS METRICS

### Before Proceeding
- [ ] Current code tested and working
- [ ] Notes updated with progress  
- [ ] No placeholder/fake code
- [ ] Hardcoded values removed

### Project Completion
- [ ] Full API per DESIGN.md
- [ ] All tests passing
- [ ] Real streaming working
- [ ] Complete documentation
- [ ] Zero known issues

## 🔥 DEVELOPMENT RULES
1. **READ EVERYTHING**: Don't skip documentation
2. **TEST CONTINUOUSLY**: Verify each step
3. **DOCUMENT EVERYTHING**: Notes are critical
4. **REAL CODE ONLY**: No shortcuts
5. **FINISH COMPLETELY**: Don't stop until working

**Goal**: Production-ready RKLLM Unix Domain Socket Server per DESIGN.md specifications.