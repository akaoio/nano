#pragma once

// IO Mapping Module - Direct RKLLM mapping
#include "handle_pool/handle_pool.h"
#include "rkllm_proxy/rkllm_proxy.h"

// Initialize mapping module
int mapping_init(void);

// Shutdown mapping module
void mapping_shutdown(void);
