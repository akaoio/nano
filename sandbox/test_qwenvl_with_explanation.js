#!/usr/bin/env node

/**
 * QwenVL Image Processing Demonstration
 * 
 * This demonstrates how to properly use QwenVL with our RKLLM server.
 * 
 * IMPORTANT: QwenVL requires TWO components:
 * 1. Vision Encoder Model (converts images to embeddings) - NOT INCLUDED
 * 2. Language Model (processes embeddings + text) - model.rkllm
 * 
 * This test shows the correct API usage but cannot fully function without
 * the vision encoder component.
 */

const path = require('path');
const fs = require('fs');

// Import test infrastructure
const ServerManager = require('../tests/lib/server-manager');
const TestClient = require('../tests/lib/test-client');

class QwenVLDemonstration {
  constructor() {
    this.serverManager = new ServerManager();
    this.client = new TestClient();
    this.handle = null;
  }

  /**
   * Explain the missing components
   */
  explainRequirements() {
    console.log('\n📚 QWENVL REQUIREMENTS EXPLANATION');
    console.log('=' .repeat(60));
    console.log('\n🔍 QwenVL is a Vision-Language model that requires:');
    console.log('\n1️⃣  Vision Encoder (MISSING):');
    console.log('   - Converts raw images to embeddings');
    console.log('   - Usually exported as ONNX then converted to RKNN');
    console.log('   - Outputs fixed-size embedding vectors');
    console.log('   - Example: vision_projector.rknn');
    
    console.log('\n2️⃣  Language Model (AVAILABLE):');
    console.log('   - Processes embeddings + text prompts');
    console.log('   - Located at: /home/x/Projects/nano/models/qwenvl/model.rkllm');
    console.log('   - Expects pre-computed image embeddings');
    
    console.log('\n3️⃣  Image Preprocessing Pipeline:');
    console.log('   - Resize image to 392x392 (QwenVL requirement)');
    console.log('   - Normalize pixel values');
    console.log('   - Run through vision encoder');
    console.log('   - Extract embedding vectors');
    
    console.log('\n❌ WITHOUT THE VISION ENCODER:');
    console.log('   - Cannot convert images to embeddings');
    console.log('   - Cannot test real image understanding');
    console.log('   - Can only demonstrate the API structure');
  }

  /**
   * Show how image preprocessing WOULD work with vision encoder
   */
  demonstrateImagePreprocessing() {
    console.log('\n🖼️  IMAGE PREPROCESSING DEMONSTRATION');
    console.log('=' .repeat(60));
    
    console.log('\n📝 If we had the vision encoder, the process would be:');
    console.log('\n// Step 1: Load and preprocess image');
    console.log(`const sharp = require('sharp'); // npm install sharp`);
    console.log(`const image = await sharp('image.jpg')
  .resize(392, 392)
  .raw()
  .toBuffer();`);
    
    console.log('\n// Step 2: Normalize pixel values');
    console.log(`const pixels = new Float32Array(image.length / 3);
for (let i = 0; i < pixels.length; i++) {
  const r = image[i * 3] / 255.0;
  const g = image[i * 3 + 1] / 255.0;
  const b = image[i * 3 + 2] / 255.0;
  
  // Apply normalization (example values)
  pixels[i * 3] = (r - 0.485) / 0.229;
  pixels[i * 3 + 1] = (g - 0.456) / 0.224;
  pixels[i * 3 + 2] = (b - 0.406) / 0.225;
}`);
    
    console.log('\n// Step 3: Run through vision encoder (MISSING)');
    console.log(`// This would require a separate RKNN model for vision encoding
// const visionModel = await loadVisionEncoder('vision_encoder.rknn');
// const embeddings = await visionModel.run(pixels);`);
    
    console.log('\n// Step 4: Format embeddings for RKLLM');
    console.log(`const multimodalInput = {
  role: "user",
  enable_thinking: false,
  input_type: 3, // RKLLM_INPUT_MULTIMODAL
  multimodal_input: {
    prompt: "What do you see in this image?",
    image_embed: embeddings, // From vision encoder
    n_image_tokens: 256,
    n_image: 1,
    image_width: 392,
    image_height: 392
  }
};`);
  }

  /**
   * Demonstrate the correct API usage
   */
  async demonstrateAPI() {
    console.log('\n🔧 API USAGE DEMONSTRATION');
    console.log('=' .repeat(60));
    
    try {
      // Initialize model with vision-language settings
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
        is_async: true,
        img_start: "<image>",  // QwenVL image tokens
        img_end: "</image>",
        img_content: "image",
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
      
      console.log('\n✅ Model parameters configured for vision-language:');
      console.log('   - img_start: "<image>"');
      console.log('   - img_end: "</image>"');
      console.log('   - img_content: "image"');
      
      // Show the multimodal input structure
      console.log('\n📋 Multimodal input structure:');
      const exampleInput = {
        role: "user",
        enable_thinking: false,
        input_type: 3, // RKLLM_INPUT_MULTIMODAL
        multimodal_input: {
          prompt: "What is in this image?",
          image_embed: "/* Float array from vision encoder */",
          n_image_tokens: 256,
          n_image: 1,
          image_width: 392,
          image_height: 392
        }
      };
      
      console.log(JSON.stringify(exampleInput, null, 2));
      
      return true;
    } catch (error) {
      console.error('❌ Demonstration failed:', error.message);
      return false;
    }
  }

  /**
   * Try to run with text-only fallback
   */
  async testTextOnlyMode() {
    console.log('\n🔤 TESTING TEXT-ONLY MODE');
    console.log('=' .repeat(60));
    console.log('📝 Since vision encoder is missing, testing text-only generation...');
    
    try {
      // Create default parameters
      const defaultParams = await this.client.sendRequest('rkllm.createDefaultParam', []);
      if (defaultParams.error) {
        throw new Error(`Failed to create default params: ${defaultParams.error.message}`);
      }
      
      // Initialize model
      const modelParams = {
        model_path: "/home/x/Projects/nano/models/qwenvl/model.rkllm",
        max_context_len: 512,
        max_new_tokens: 128,
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
        is_async: true,
        img_start: "<image>",
        img_end: "</image>",
        img_content: "image",
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
      
      console.log('🔧 Initializing QwenVL model...');
      const initResponse = await this.client.sendRequest('rkllm.init', [
        null,
        modelParams,
        null
      ]);
      
      if (initResponse.error) {
        throw new Error(`Model init failed: ${initResponse.error.message}`);
      }
      
      console.log('✅ Model initialized successfully');
      
      // Test text-only generation
      const textInput = {
        role: "user",
        enable_thinking: false,
        input_type: 0, // RKLLM_INPUT_PROMPT
        prompt_input: "Explain what a vision-language model is and how it processes images."
      };
      
      const inferParams = {
        mode: 0, // RKLLM_INFER_GENERATE
        lora_params: null,
        prompt_cache_params: null,
        keep_history: 1
      };
      
      console.log('\n🚀 Sending text-only request...');
      const response = await this.client.sendRealTimeStreamingRequest('rkllm.run_async', [
        null,
        textInput,
        inferParams,
        null
      ]);
      
      console.log('\n\n📊 RESULTS:');
      console.log('=' .repeat(40));
      console.log(`📝 Generated ${response.tokens.length} tokens`);
      
      if (response.finalPerf) {
        console.log(`⚡ Generation time: ${response.finalPerf.generate_time_ms}ms`);
        console.log(`📈 Speed: ${(response.finalPerf.generate_tokens / response.finalPerf.generate_time_ms * 1000).toFixed(2)} tokens/sec`);
      }
      
      return true;
      
    } catch (error) {
      console.error('❌ Text-only test failed:', error.message);
      return false;
    } finally {
      // Cleanup
      try {
        await this.client.sendRequest('rkllm.destroy', [null]);
      } catch (e) {
        // Ignore cleanup errors
      }
    }
  }

  /**
   * Create a complete working example
   */
  createWorkingExample() {
    console.log('\n📝 COMPLETE WORKING EXAMPLE');
    console.log('=' .repeat(60));
    
    const exampleCode = `
// Complete QwenVL Integration Example
// ==================================

const sharp = require('sharp');
const { loadRKNNModel } = require('rknn-node'); // Hypothetical RKNN binding

class QwenVLProcessor {
  constructor() {
    this.visionEncoder = null;
    this.rkllmClient = null;
  }
  
  async initialize() {
    // 1. Load vision encoder (MISSING - needs separate RKNN model)
    // this.visionEncoder = await loadRKNNModel('vision_encoder.rknn');
    
    // 2. Connect to RKLLM server
    this.rkllmClient = new TestClient();
    await this.rkllmClient.connect();
    
    // 3. Initialize QwenVL language model
    const params = {
      model_path: "/home/x/Projects/nano/models/qwenvl/model.rkllm",
      img_start: "<image>",
      img_end: "</image>",
      img_content: "image",
      // ... other parameters
    };
    
    await this.rkllmClient.sendRequest('rkllm.init', [null, params, null]);
  }
  
  async processImage(imagePath, prompt) {
    // 1. Load and preprocess image
    const imageBuffer = await sharp(imagePath)
      .resize(392, 392)
      .raw()
      .toBuffer();
    
    // 2. Convert to normalized float array
    const pixels = this.normalizePixels(imageBuffer);
    
    // 3. Get embeddings from vision encoder (MISSING)
    // const embeddings = await this.visionEncoder.run(pixels);
    
    // 4. Create multimodal input
    const input = {
      role: "user",
      enable_thinking: false,
      input_type: 3, // RKLLM_INPUT_MULTIMODAL
      multimodal_input: {
        prompt: prompt,
        image_embed: embeddings, // From vision encoder
        n_image_tokens: 256,
        n_image: 1,
        image_width: 392,
        image_height: 392
      }
    };
    
    // 5. Run inference
    const response = await this.rkllmClient.sendStreamingRequest(
      'rkllm.run_async',
      [null, input, { mode: 0 }, null]
    );
    
    return response.fullText;
  }
  
  normalizePixels(buffer) {
    // Image normalization logic here
    // ...
  }
}

// Usage:
const processor = new QwenVLProcessor();
await processor.initialize();
const description = await processor.processImage('cat.jpg', 'What animal is this?');
console.log(description);
`;

    console.log(exampleCode);
  }

  /**
   * Run the complete demonstration
   */
  async runDemonstration() {
    console.log('🚀 QwenVL Integration Demonstration');
    console.log('=' .repeat(60));
    console.log('📝 This demonstrates the CORRECT way to use QwenVL with RKLLM');
    console.log('⚠️  Note: Full functionality requires vision encoder component');
    
    try {
      // Start server
      console.log('\n1️⃣ Starting RKLLM server...');
      await this.serverManager.startServer();
      await new Promise(resolve => setTimeout(resolve, 2000));
      
      // Connect client
      console.log('\n2️⃣ Connecting to server...');
      await this.client.connect();
      await new Promise(resolve => setTimeout(resolve, 1000));
      
      // Explain requirements
      this.explainRequirements();
      
      // Show preprocessing steps
      this.demonstrateImagePreprocessing();
      
      // Demonstrate API
      await this.demonstrateAPI();
      
      // Test text-only mode
      await this.testTextOnlyMode();
      
      // Show complete example
      this.createWorkingExample();
      
      console.log('\n✅ Demonstration completed!');
      console.log('\n📌 NEXT STEPS:');
      console.log('1. Obtain vision encoder model for QwenVL');
      console.log('2. Convert it to RKNN format');
      console.log('3. Implement image preprocessing pipeline');
      console.log('4. Use the multimodal API as demonstrated');
      
      return true;
      
    } catch (error) {
      console.error('\n❌ Demonstration failed:', error.message);
      return false;
    } finally {
      // Cleanup
      console.log('\n🧹 Cleaning up...');
      this.client.disconnect();
      this.serverManager.stopServer();
      console.log('🏁 Cleanup completed');
    }
  }
}

// Run the demonstration
if (require.main === module) {
  const demo = new QwenVLDemonstration();
  demo.runDemonstration().then((success) => {
    process.exit(success ? 0 : 1);
  }).catch((error) => {
    console.error('\n💥 Demonstration crashed:', error);
    process.exit(1);
  });
}

module.exports = QwenVLDemonstration;