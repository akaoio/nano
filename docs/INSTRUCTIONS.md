# Development Instructions for RKLLM Unix Domain Socket Server

## 🔥 CRITICAL: READ BEFORE STARTING

### Required Reading Order
1. **MUST READ**: `docs/DESIGN.md` - Complete design document (every single line)
2. **MUST READ**: `docs/notes/` - All existing notes to understand current project status
3. **MUST READ**: This document (`docs/PROMPT.md`)

### Documentation Standards
- **Always take notes**: Create `docs/notes/YYYYMMDD_HH-MM_<descriptive-header>.md` 
- **Always reference back**: When confused, re-read DESIGN.md and existing notes
- **Document issues**: Create `docs/issues/YYYYMMDD_HH-MM_<issue-header>.md` for any problems

## 🎯 PROJECT COMPLETION CRITERIA

**The project is ONLY finished when:**
- ✅ Server runs without errors
- ✅ Complete test suite passes (`npm test`)
- ✅ Real input/output/expected results are clearly demonstrated
- ✅ All functionality works as designed in DESIGN.md

## 📋 DEVELOPMENT WORKFLOW

### Step-by-Step Process
1. **Read Documentation**: DESIGN.md + existing notes
2. **Take Notes**: Document current understanding and plan
3. **Implement**: Write real, production-ready code
4. **Test**: Verify functionality before moving forward
5. **Document**: Update notes with progress and findings
6. **Repeat**: Continue until project completion

### Code Quality Standards
- ❌ **NEVER CREATE**: Fake code, TODO placeholders, "FOR NOW" implementations
- ❌ **NEVER HARDCODE**: Paths, values, or configuration
- ✅ **ALWAYS IMPLEMENT**: Real, production-ready code
- ✅ **ALWAYS VERIFY**: All code works before proceeding

## 🗂️ FILE ORGANIZATION

### Development Files
- **Temporary/Test Files**: `sandbox/` directory ONLY
- **Official Tests**: `tests/` directory (run via `npm test`)
- **Documentation**: `docs/notes/` and `docs/issues/`

### Naming Convention
- **Documentation**: `YYYYMMDD_HH-MM_<descriptive-header>.md`
- **Example**: `20250723_14-30_implementing-json-rpc-parser.md`

## 🔗 EXTERNAL DEPENDENCIES

### Required Libraries
- **RKLLM Library**: https://github.com/airockchip/rknn-llm
  - Download: `rkllm.h` and `librkllmrt.so`
- **JSON-C Library**: https://github.com/json-c/json-c
  - For JSON parsing in C

### Test Models (DO NOT HARDCODE PATHS)
1. **Standard Testing**:
   - Path: `/home/x/Projects/nano/models/qwen3/model.rkllm`
   - Use: General functionality testing

2. **LoRa Testing**:
   - Base Model: `/home/x/Projects/nano/models/lora/model.rkllm`
   - LoRa File: `/home/x/Projects/nano/models/lora/lora.rkllm`
   - Use: LoRa adapter functionality testing

## 🧪 TESTING GUIDELINES

### Server Testing Process
- **⚠️ IMPORTANT**: Never start server directly (causes hanging)
- **✅ CORRECT METHOD**: Start server from within Node.js/Python test scripts
- **✅ CONCURRENT TESTING**: Run server and tests together for efficient testing

### Testing Requirements
- **Programmatic Tests**: Automated, verifiable results
- **Strong Results**: Clear pass/fail criteria
- **Technical Proof**: Measurable input/output validation

## 🚨 ISSUE HANDLING

When encountering problems:
1. **Document Immediately**: Create issue file in `docs/issues/`
2. **Use Format**: `YYYYMMDD_HH-MM_<issue-description>.md`
3. **Include Details**: 
   - What went wrong
   - Steps to reproduce
   - Expected vs actual behavior
   - Solution attempts

## 📝 DEVELOPMENT NOTES TEMPLATE

```markdown
# YYYYMMDD_HH-MM_<Task Description>

## Current Status
- What I'm working on
- What's been completed

## Plan
- [ ] Step 1
- [ ] Step 2
- [ ] Step 3

## Implementation Details
- Key decisions made
- Technical approach

## Testing Results
- What was tested
- Results obtained

## Next Steps
- What to do next
- Dependencies or blockers
```

## 🎯 SUCCESS METRICS

### Before Moving Forward
- [ ] Current implementation tested and verified
- [ ] Notes updated with progress
- [ ] No fake/placeholder code exists
- [ ] All hardcoded values removed
- [ ] Test results documented

### Project Completion
- [ ] Full RKLLM API implemented per DESIGN.md
- [ ] All tests pass consistently
- [ ] Real streaming functionality working
- [ ] Complete documentation updated
- [ ] No known issues remaining

---

## 🔥 FINAL REMINDERS

1. **READ EVERYTHING**: Don't skip any documentation
2. **TEST CONTINUOUSLY**: Verify each step before proceeding  
3. **DOCUMENT EVERYTHING**: Notes are your memory
4. **REAL CODE ONLY**: No shortcuts or placeholders
5. **FINISH COMPLETELY**: Don't stop until everything works

**Remember: The goal is a production-ready RKLLM Unix Domain Socket Server that works perfectly according to the specifications in DESIGN.md.**