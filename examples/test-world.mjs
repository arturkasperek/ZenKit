#!/usr/bin/env node

// Test World properties and functionality
import ZenKitModule from './zenkit.mjs';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

async function testWorld() {
    try {
        console.log('🌍 Testing World Properties...\n');

        const ZenKit = await ZenKitModule();
        console.log(`📚 ZenKit Version: ${ZenKit.getZenKitVersion()}\n`);

        const zenFilePath = path.join(__dirname, '..', '..', 'TOTENINSEL.ZEN');
        
        if (!fs.existsSync(zenFilePath)) {
            console.log('⚠️  TOTENINSEL.ZEN not found, please ensure it exists in the project root');
            return;
        }

        console.log(`📁 Loading ZEN file: ${path.basename(zenFilePath)}`);
        const fileBuffer = fs.readFileSync(zenFilePath);
        const uint8Array = new Uint8Array(fileBuffer);
        const wasmMemory = ZenKit._malloc(uint8Array.length);
        ZenKit.HEAPU8.set(uint8Array, wasmMemory);
        
        try {
            // Create and load world
            const world = ZenKit.createWorld();
            console.log('🚀 Loading world...');
            const result = world.load(wasmMemory, uint8Array.length);
            
            if (!result.success) {
                console.log(`❌ Failed to load: ${result.errorMessage}`);
                return;
            }
            
            console.log('✅ World loaded successfully!\n');
            
            // Test property-based access (not function calls!)
            console.log('🏷️ World Properties (property access, not functions):');
            console.log(`   • world.npcSpawnEnabled: ${world.npcSpawnEnabled}`);
            console.log(`   • world.npcSpawnFlags: ${world.npcSpawnFlags}`);
            console.log(`   • world.hasPlayer: ${world.hasPlayer}`);
            console.log(`   • world.hasSkyController: ${world.hasSkyController}`);
            
            // Access mesh through world property
            console.log('\n🔗 World → Mesh Relationship:');
            const mesh = world.mesh;
            console.log(`   • world.mesh: ${mesh.constructor.name}`);
            console.log(`   • world.mesh.name: "${mesh.name}"`);
            
            // Show mesh basic info through world
            console.log('\n📊 Mesh Data Summary (via world.mesh):');
            console.log(`   • vertices: ${mesh.vertices.size()} items`);
            console.log(`   • features: ${mesh.features.size()} items`);
            console.log(`   • indices: ${mesh.vertexIndices.size()} items`);
            
            // Bounding box info
            const bbox_min = mesh.boundingBoxMin;
            const bbox_max = mesh.boundingBoxMax;
            console.log('\n📏 World Bounding Box:');
            console.log(`   • min: {x: ${bbox_min.x.toFixed(1)}, y: ${bbox_min.y.toFixed(1)}, z: ${bbox_min.z.toFixed(1)}}`);
            console.log(`   • max: {x: ${bbox_max.x.toFixed(1)}, y: ${bbox_max.y.toFixed(1)}, z: ${bbox_max.z.toFixed(1)}}`);
            
            // Calculate world dimensions
            const width = bbox_max.x - bbox_min.x;
            const height = bbox_max.y - bbox_min.y;
            const depth = bbox_max.z - bbox_min.z;
            console.log(`   • dimensions: ${width.toFixed(1)} × ${height.toFixed(1)} × ${depth.toFixed(1)}`);
            
            console.log('\n🎯 Key Benefits Demonstrated:');
            console.log('   ✅ Property access: world.hasPlayer (not world.hasPlayer())');
            console.log('   ✅ Direct mesh access: world.mesh');
            console.log('   ✅ Clean API: world.npcSpawnEnabled vs getNpcSpawnEnabled()');
            console.log('   ✅ Natural JavaScript: mesh.vertices.size() calculates count');
            
        } finally {
            ZenKit._free(wasmMemory);
        }
        
    } catch (error) {
        console.error('💥 World test failed:', error.message);
        process.exit(1);
    }
}

testWorld();
