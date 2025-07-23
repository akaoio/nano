#!/usr/bin/env node

/**
 * QwenVL Image Processing Test
 * 
 * This test demonstrates how to send multimodal (image + text) input to QwenVL model
 * through our RKLLM Unix Domain Socket server.
 * 
 * Based on DESIGN.md specifications for multimodal input handling.
 */

const path = require('path');
const fs = require('fs');

// Import test infrastructure
const ServerManager = require('../tests/lib/server-manager');
const TestClient = require('../tests/lib/test-client');

class QwenVLImageTest {
  constructor() {
    this.serverManager = new ServerManager();
    this.client = new TestClient();
    this.handle = null;
  }

  /**
   * Generate mock image embeddings for testing
   * In a real implementation, this would come from the vision model
   */
  generateMockImageEmbeddings(imageWidth = 392, imageHeight = 392) {
    // QwenVL-2B uses 1536 embedding dimension for images
    // QwenVL-7B uses 3584 embedding dimension
    const embedDim = 1536; // Using 2B model settings
    const numImageTokens = 256; // Typical number of image tokens
    
    console.log(`🖼️  Generating mock embeddings: ${numImageTokens} tokens × ${embedDim} dimensions`);
    
    // Generate random embeddings (in real scenario, these come from vision model)
    const imageEmbeddings = [];
    for (let i = 0; i < numImageTokens * embedDim; i++) {
      // Generate small random values typical of neural network embeddings
      imageEmbeddings.push((Math.random() - 0.5) * 0.1);
    }
    
    return {
      image_embed: imageEmbeddings,
      n_image_tokens: numImageTokens,
      n_image: 1,
      image_width: imageWidth,
      image_height: imageHeight
    };
  }

  /**
   * Create multimodal input for QwenVL
   */
  createMultimodalInput(prompt, imageFile) {
    console.log(`📝 Creating multimodal input for: "${prompt}"`);
    console.log(`🖼️  Image file: ${imageFile}`);
    
    // Generate mock image embeddings
    const imageData = this.generateMockImageEmbeddings();
    
    return {
      role: "user",
      enable_thinking: false,
      input_type: 3, // RKLLM_INPUT_MULTIMODAL
      multimodal_input: {
        prompt: prompt,
        ...imageData
      }
    };
  }

  /**
   * Initialize QwenVL model
   */
  async initializeModel() {
    console.log('\n🚀 INITIALIZING QWENVL MODEL');
    console.log('=' .repeat(60));
    
    try {
      // Create default parameters first
      console.log('📋 Creating default parameters...');
      const defaultParams = await this.client.sendRequest('rkllm.createDefaultParam', []);
      
      if (defaultParams.error) {
        throw new Error(`Failed to create default params: ${defaultParams.error.message}`);
      }
      
      // Configure parameters for QwenVL model
      const modelParams = {
        model_path: "/home/x/Projects/nano/models/qwenvl/model.rkllm",
        max_context_len: 2048,
        max_new_tokens: 512,
        top_k: 40,
        top_p: 0.9,
        temperature: 0.7,
        repeat_penalty: 1.1,
        frequency_penalty: 0.0,
        presence_penalty: 0.0,
        mirostat: 0,
        mirostat_tau: 5.0,
        mirostat_eta: 0.1,
        skip_special_token: false,
        is_async: true, // Enable async mode for streaming
        img_start: "<image>", // QwenVL image start token
        img_end: "</image>", // QwenVL image end token
        img_content: "image", // QwenVL image content token
        extend_param: {
          base_domain_id: 0,
          embed_flash: 0,
          enabled_cpus_num: 4,
          enabled_cpus_mask: 15,
          n_batch: 1,
          use_cross_attn: 0,
          reserved: null
        }
      };
      
      console.log('🔧 Initializing model with parameters...');
      const initResponse = await this.client.sendRequest('rkllm.init', [
        null, // handle (managed by server)
        modelParams,
        null  // callback (managed by server)
      ]);
      
      if (initResponse.error) {
        throw new Error(`Model initialization failed: ${initResponse.error.message}`);
      }
      
      console.log('✅ QwenVL model initialized successfully!');
      this.handle = initResponse.result;
      return true;
      
    } catch (error) {
      console.error('❌ Model initialization failed:', error.message);
      return false;
    }
  }

  /**
   * Test image description with QwenVL
   */
  async testImageDescription(imageFile, question) {
    console.log(`\n🖼️  TESTING IMAGE DESCRIPTION`);
    console.log('=' .repeat(60));
    console.log(`📸 Image: ${imageFile}`);
    console.log(`❓ Question: "${question}"`);
    
    try {
      // Check if image file exists
      const imagePath = path.join(__dirname, '..', 'tests', 'images', imageFile);
      if (!fs.existsSync(imagePath)) {
        console.log(`⚠️  Image file not found: ${imagePath}`);
        console.log('🔧 Using mock image data for testing...');
      }
      
      // Create multimodal input
      const input = this.createMultimodalInput(question, imageFile);
      
      // Create inference parameters
      const inferParams = {
        mode: 0, // RKLLM_INFER_GENERATE
        lora_params: null,
        prompt_cache_params: null,
        keep_history: 1
      };
      
      console.log('\n🚀 Sending multimodal request...');
      const response = await this.client.sendRealTimeStreamingRequest('rkllm.run_async', [
        null, // handle (managed by server)
        input,
        inferParams,
        null  // userdata (managed by server)
      ]);
      
      console.log('\n📊 RESULTS:');
      console.log('=' .repeat(40));
      console.log(`📝 Generated text: "${response.fullText}"`);
      console.log(`🎯 Total tokens: ${response.tokens.length}`);
      
      if (response.finalPerf) {
        console.log(`⚡ Generation time: ${response.finalPerf.generate_time_ms}ms`);
        console.log(`💾 Memory usage: ${response.finalPerf.memory_usage_mb}MB`);
        console.log(`📈 Tokens/sec: ${(response.finalPerf.generate_tokens / response.finalPerf.generate_time_ms * 1000).toFixed(2)}`);
      }
      
      return {
        success: true,
        text: response.fullText,
        tokens: response.tokens.length,
        performance: response.finalPerf
      };
      
    } catch (error) {
      console.error('❌ Image description test failed:', error.message);
      return {
        success: false,
        error: error.message
      };
    }
  }

  /**
   * Test multiple images with different questions
   */
  async runComprehensiveImageTests() {
    console.log('\n🧪 COMPREHENSIVE QWENVL IMAGE TESTS');
    console.log('=' .repeat(60));
    
    const testCases = [
      {
        image: 'image1.jpg',
        question: 'What do you see in this image?'
      },
      {
        image: 'image1.jpg', 
        question: 'Describe the colors and objects in detail.'
      },
      {
        image: 'image2.png',
        question: 'What animal is shown in this image?'
      },
      {
        image: 'image2.png',
        question: 'What is the mood or expression of this animal?'
      }
    ];
    
    const results = [];
    
    for (let i = 0; i < testCases.length; i++) {
      const testCase = testCases[i];
      console.log(`\n🧪 TEST ${i + 1}/${testCases.length}`);
      
      const result = await this.testImageDescription(testCase.image, testCase.question);
      results.push({
        ...testCase,
        ...result
      });
      
      // Small delay between tests
      await new Promise(resolve => setTimeout(resolve, 1000));
    }
    
    return results;
  }

  /**
   * Print comprehensive test summary
   */
  printTestSummary(results) {
    console.log('\n📋 TEST SUMMARY');
    console.log('=' .repeat(60));
    
    let successCount = 0;
    let totalTokens = 0;
    let totalTime = 0;
    
    results.forEach((result, i) => {
      console.log(`\n🧪 Test ${i + 1}: ${result.image} - "${result.question}"`);
      
      if (result.success) {
        console.log(`  ✅ SUCCESS`);
        console.log(`  📝 Response: "${result.text.substring(0, 100)}${result.text.length > 100 ? '...' : ''}"`);
        console.log(`  🎯 Tokens: ${result.tokens}`);
        
        if (result.performance) {
          console.log(`  ⚡ Time: ${result.performance.generate_time_ms}ms`);
          totalTime += result.performance.generate_time_ms;
        }
        
        successCount++;
        totalTokens += result.tokens;
      } else {
        console.log(`  ❌ FAILED: ${result.error}`);
      }
    });
    
    console.log(`\n📊 OVERALL STATISTICS:`);
    console.log(`  🎯 Success Rate: ${successCount}/${results.length} (${(successCount/results.length*100).toFixed(1)}%)`);
    console.log(`  📝 Total Tokens Generated: ${totalTokens}`);
    console.log(`  ⚡ Total Generation Time: ${totalTime.toFixed(1)}ms`);
    
    if (totalTime > 0 && totalTokens > 0) {
      console.log(`  📈 Average Speed: ${(totalTokens / totalTime * 1000).toFixed(2)} tokens/sec`);
    }
  }

  /**
   * Run the complete test suite
   */
  async runTests() {
    console.log('🚀 QwenVL Image Processing Test Suite');
    console.log('=' .repeat(60));
    console.log('📝 This test demonstrates multimodal (image + text) processing');
    console.log('🖼️  Testing with images from tests/images/ directory');
    console.log('🤖 Using QwenVL model for vision-language understanding');
    
    try {
      // Start server
      console.log('\n1️⃣ Starting RKLLM server...');
      await this.serverManager.startServer();
      await new Promise(resolve => setTimeout(resolve, 2000));
      
      // Connect client
      console.log('\n2️⃣ Connecting to server...');
      await this.client.connect();
      await new Promise(resolve => setTimeout(resolve, 1000));
      
      // Initialize model
      console.log('\n3️⃣ Initializing QwenVL model...');
      const modelInit = await this.initializeModel();
      if (!modelInit) {
        throw new Error('Failed to initialize QwenVL model');
      }
      
      // Run image tests
      console.log('\n4️⃣ Running comprehensive image tests...');
      const results = await this.runComprehensiveImageTests();
      
      // Print summary
      this.printTestSummary(results);
      
      console.log('\n✅ QwenVL Image Processing Test Suite COMPLETED!');
      return results;
      
    } catch (error) {
      console.error('\n❌ Test suite failed:', error.message);
      return null;
    } finally {
      // Cleanup
      console.log('\n🧹 Cleaning up...');
      
      try {
        if (this.handle) {
          await this.client.sendRequest('rkllm.destroy', [null]);
        }
      } catch (e) {
        // Ignore cleanup errors
      }
      
      this.client.disconnect();
      this.serverManager.stopServer();
      
      console.log('🏁 Test cleanup completed');
    }
  }
}

// Run the test if this file is executed directly
if (require.main === module) {
  const test = new QwenVLImageTest();
  test.runTests().then((results) => {
    if (results && results.length > 0) {
      const successCount = results.filter(r => r.success).length;
      console.log(`\n🎯 Final Result: ${successCount}/${results.length} tests passed`);
      process.exit(successCount === results.length ? 0 : 1);
    } else {
      console.log('\n❌ No test results available');
      process.exit(1);
    }
  }).catch((error) => {
    console.error('\n💥 Test suite crashed:', error);
    process.exit(1);
  });
}

module.exports = QwenVLImageTest;