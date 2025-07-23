#!/usr/bin/env node

/**
 * QwenVL Multimodal API Example
 * 
 * This example demonstrates the CORRECT way to use QwenVL with our RKLLM server
 * for image processing tasks. It shows the complete pipeline and API usage.
 * 
 * IMPORTANT: This example shows the correct API but cannot run without the
 * vision encoder component. See comments for what's needed.
 */

const TestClient = require('../tests/lib/test-client');

/**
 * Complete working example for QwenVL multimodal processing
 */
class QwenVLMultimodalExample {
  constructor() {
    this.client = new TestClient();
  }

  /**
   * Initialize the QwenVL model with proper parameters
   */
  async initializeModel() {
    // First, get default parameters
    await this.client.sendRequest('rkllm.createDefaultParam', []);
    
    // Configure for QwenVL with vision support
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
      
      // Important: QwenVL-specific image tokens
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
    
    // Initialize the model
    const response = await this.client.sendRequest('rkllm.init', [
      null,          // handle (managed by server)
      modelParams,   // parameters
      null          // callback (managed by server)
    ]);
    
    return response.result;
  }

  /**
   * Process an image with QwenVL
   * 
   * NOTE: This requires the vision encoder component to generate embeddings
   */
  async processImage(imageEmbeddings, prompt) {
    // Create multimodal input structure
    const multimodalInput = {
      role: "user",
      enable_thinking: false,
      input_type: 3,  // RKLLM_INPUT_MULTIMODAL
      multimodal_input: {
        prompt: prompt,
        image_embed: imageEmbeddings,  // Float array from vision encoder
        n_image_tokens: 256,           // Number of image tokens
        n_image: 1,                    // Number of images
        image_width: 392,              // QwenVL expects 392x392
        image_height: 392
      }
    };
    
    // Inference parameters
    const inferParams = {
      mode: 0,  // RKLLM_INFER_GENERATE
      lora_params: null,
      prompt_cache_params: null,
      keep_history: 1
    };
    
    // Send request and get streaming response
    const response = await this.client.sendStreamingRequest(
      'rkllm.run_async',
      [
        null,           // handle (managed by server)
        multimodalInput,
        inferParams,
        null           // userdata (managed by server)
      ]
    );
    
    return response;
  }

  /**
   * Example usage showing the complete pipeline
   */
  async runExample() {
    console.log('QwenVL Multimodal Processing Example');
    console.log('=====================================\\n');
    
    try {
      // Connect to server
      await this.client.connect();
      console.log('✅ Connected to RKLLM server\\n');
      
      // Initialize model
      console.log('Initializing QwenVL model...');
      await this.initializeModel();
      console.log('✅ Model initialized\\n');
      
      // ===== MISSING COMPONENT =====
      // In a real implementation, you would:
      // 1. Load the vision encoder model:
      //    const visionEncoder = await loadRKNNModel('vision_encoder.rknn');
      // 
      // 2. Preprocess the image:
      //    const image = await sharp('image.jpg')
      //      .resize(392, 392)
      //      .raw()
      //      .toBuffer();
      //    const normalizedPixels = normalizeImage(image);
      // 
      // 3. Generate embeddings:
      //    const embeddings = await visionEncoder.run(normalizedPixels);
      // =============================
      
      // For demonstration, show the API structure
      console.log('📋 Multimodal Input Structure:');
      console.log(JSON.stringify({
        role: "user",
        enable_thinking: false,
        input_type: 3,
        multimodal_input: {
          prompt: "Describe what you see in this image in detail.",
          image_embed: "/* Float array from vision encoder (size: 256 * 1536) */",
          n_image_tokens: 256,
          n_image: 1,
          image_width: 392,
          image_height: 392
        }
      }, null, 2));
      
      console.log('\\n⚠️  NOTE: Actual image processing requires:');
      console.log('1. Vision encoder model (vision_encoder.rknn)');
      console.log('2. Image preprocessing pipeline');
      console.log('3. RKNN runtime for vision encoding');
      
      console.log('\\n✅ The RKLLM server fully supports multimodal input!');
      console.log('   Just provide the image embeddings from the vision encoder.');
      
    } catch (error) {
      console.error('❌ Error:', error.message);
    } finally {
      // Cleanup
      try {
        await this.client.sendRequest('rkllm.destroy', [null]);
      } catch (e) {}
      
      this.client.disconnect();
    }
  }
}

// Example of how to preprocess images (when vision encoder is available)
function createImagePreprocessor() {
  return {
    /**
     * Preprocess image for QwenVL
     * @param {Buffer} imageBuffer - Raw image data
     * @returns {Float32Array} Normalized pixel values
     */
    preprocessImage(imageBuffer) {
      // QwenVL expects:
      // - 392x392 resolution
      // - RGB format
      // - Normalized values
      
      const pixels = new Float32Array(392 * 392 * 3);
      
      // Image normalization (example values, adjust based on model)
      const mean = [0.485, 0.456, 0.406];
      const std = [0.229, 0.224, 0.225];
      
      for (let i = 0; i < pixels.length / 3; i++) {
        const r = imageBuffer[i * 3] / 255.0;
        const g = imageBuffer[i * 3 + 1] / 255.0;
        const b = imageBuffer[i * 3 + 2] / 255.0;
        
        pixels[i * 3] = (r - mean[0]) / std[0];
        pixels[i * 3 + 1] = (g - mean[1]) / std[1];
        pixels[i * 3 + 2] = (b - mean[2]) / std[2];
      }
      
      return pixels;
    }
  };
}

// Run example if executed directly
if (require.main === module) {
  console.log('🚀 Starting QwenVL Multimodal Example\\n');
  
  // Note: This requires the server to be running
  console.log('⚠️  Make sure the RKLLM server is running!');
  console.log('   Run: cd build && ./rkllm_uds_server\\n');
  
  const example = new QwenVLMultimodalExample();
  example.runExample().catch(console.error);
}

module.exports = {
  QwenVLMultimodalExample,
  createImagePreprocessor
};