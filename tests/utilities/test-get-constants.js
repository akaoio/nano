const TestClient = require('../lib/test-client');
const { printTestSection, printTestResult, RKLLM_CONSTANTS } = require('../lib/test-helpers');

/**
 * Test rkllm.get_constants
 * Tests access to RKLLM constants and enums per DESIGN.md lines 271-295
 */
async function testGetConstants(client) {
  printTestSection('TESTING: rkllm.get_constants');

  console.log('📋 EXPECTED:');
  console.log('   - Method should return all RKLLM constants');
  console.log('   - Should include CPU_MASKS, LLM_CALL_STATES, INPUT_TYPES, INFER_MODES');
  console.log('   - Values should match DESIGN.md specification');

  const result = await client.testFunction('rkllm.get_constants', []);

  if (result.success && result.response.result) {
    const constants = result.response.result;
    
    console.log('\\n🔍 RECEIVED CONSTANTS:');
    console.log(JSON.stringify(constants, null, 2));

    // Verify required constant groups
    const requiredGroups = ['CPU_MASKS', 'LLM_CALL_STATES', 'INPUT_TYPES', 'INFER_MODES'];
    let allGroupsPresent = true;

    for (const group of requiredGroups) {
      if (!(group in constants)) {
        console.log(`❌ Missing constant group: ${group}`);
        allGroupsPresent = false;
      } else {
        console.log(`✅ Found constant group: ${group}`);
      }
    }

    // Verify specific constant values match DESIGN.md
    if (constants.INPUT_TYPES) {
      const expectedInputTypes = {
        'RKLLM_INPUT_PROMPT': 0,
        'RKLLM_INPUT_TOKEN': 1,
        'RKLLM_INPUT_EMBED': 2,
        'RKLLM_INPUT_MULTIMODAL': 3
      };

      let inputTypesCorrect = true;
      for (const [key, expectedValue] of Object.entries(expectedInputTypes)) {
        if (constants.INPUT_TYPES[key] !== expectedValue) {
          console.log(`❌ INPUT_TYPES.${key}: expected ${expectedValue}, got ${constants.INPUT_TYPES[key]}`);
          inputTypesCorrect = false;
        }
      }

      if (inputTypesCorrect) {
        console.log('✅ INPUT_TYPES values match DESIGN.md specification');
      }
    }

    if (constants.LLM_CALL_STATES) {
      const expectedCallStates = {
        'RKLLM_RUN_NORMAL': 0,
        'RKLLM_RUN_WAITING': 1,
        'RKLLM_RUN_FINISH': 2,
        'RKLLM_RUN_ERROR': 3
      };

      let callStatesCorrect = true;
      for (const [key, expectedValue] of Object.entries(expectedCallStates)) {
        if (constants.LLM_CALL_STATES[key] !== expectedValue) {
          console.log(`❌ LLM_CALL_STATES.${key}: expected ${expectedValue}, got ${constants.LLM_CALL_STATES[key]}`);
          callStatesCorrect = false;
        }
      }

      if (callStatesCorrect) {
        console.log('✅ LLM_CALL_STATES values match DESIGN.md specification');
      }
    }

    printTestResult('rkllm.get_constants', allGroupsPresent);
    return allGroupsPresent;
  } else {
    printTestResult('rkllm.get_constants', false, result.error || 'No constants returned');
    return false;
  }
}

module.exports = testGetConstants;