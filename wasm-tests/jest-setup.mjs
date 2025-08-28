/**
 * Jest Setup File for ZenKit WASM Tests
 *
 * This file sets up the test environment to use the real ZenKit WASM module.
 */

import path from 'path';
import fs from 'fs';

// Mock Node.js globals for browser-like environment
if (typeof window === 'undefined') {
    global.window = {};
    global.self = global;
}

// Global ZenKit instance
global.zenkitInstance = null;
global.zenkitModule = null;

// Helper functions for tests
global.setupZenKit = async () => {
    if (global.zenkitInstance) {
        return global.zenkitInstance;
    }

    try {
        // Load the real WASM module directly
        const wasmPath = path.join(process.cwd(), 'build-wasm', 'wasm', 'zenkit.mjs');
        const wasmBinaryPath = path.join(process.cwd(), 'build-wasm', 'wasm', 'zenkit.wasm');

        // Check if WASM files exist
        if (!fs.existsSync(wasmPath) || !fs.existsSync(wasmBinaryPath)) {
            throw new Error(`WASM module not found! Please build it first:
  Expected JS module at: ${wasmPath}
  Expected WASM binary at: ${wasmBinaryPath}
  
Build commands:
  npm run build:wasm`);
        }

        console.log('🔍 Loading real ZenKit WASM module for Jest...');
        
        // Use dynamic import to load the WASM module
        const wasmUrl = `file://${wasmPath}`;
        const ZenKitModule = await import(wasmUrl);
        
        // Initialize the WASM module
        global.zenkitInstance = await ZenKitModule.default();
        global.zenkitModule = ZenKitModule;
        
        console.log(`✅ Real ZenKit WASM module loaded: ${global.zenkitInstance.getZenKitVersion()}`);
        
        return global.zenkitInstance;
    } catch (error) {
        console.error('💥 Cannot load real WASM module for Jest tests');
        console.error('   Make sure to build the WASM module first:');
        console.error('   npm run build:wasm');
        console.error('   Error:', error.message);
        throw error;
    }
};

global.cleanupZenKit = () => {
    // Cleanup if needed
};

global.getTestDataPath = (filename) => {
    return path.join(process.cwd(), 'tests', 'samples', filename);
};

global.loadFileIntoWasm = (filepath, zenkitInstance) => {
    if (!fs.existsSync(filepath)) {
        throw new Error(`Test file not found: ${filepath}`);
    }

    const fileBuffer = fs.readFileSync(filepath);
    const uint8Array = new Uint8Array(fileBuffer);

    // Access WASM memory functions through the zenkit instance (which is the actual WASM module)
    const wasmMemory = global.zenkitInstance._malloc(uint8Array.length);
    global.zenkitInstance.HEAPU8.set(uint8Array, wasmMemory);

    return {
        pointer: wasmMemory,
        size: uint8Array.length,
        cleanup: () => global.zenkitInstance._free(wasmMemory)
    };
};