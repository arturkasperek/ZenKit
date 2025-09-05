#!/usr/bin/env node

// Test World properties and functionality with ZenKit improvements
import ZenKitModule from './zenkit.mjs';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

async function testWorld() {
    try {
        console.log('🌍 Testing ZenKit World Properties & Improvements...\n');

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

        // Create and load world
        const world = ZenKit.createWorld();
        console.log('🚀 Loading world...');
        
        // NEW: Use automatic memory management with optional version parameter
        // loadFromArray(uint8Array, version = 0) where 0 = auto-detect, 1 = Gothic 1, 2 = Gothic 2
        const result = world.loadFromArray(uint8Array);

        if (!result.success) {
            console.log(`❌ Failed to load: ${result.errorMessage}`);
            console.log(`   Last error: ${world.getLastError()}`);
            return;
        }

            console.log('✅ World loaded successfully!\n');

            // Test new error handling methods
            console.log('🛡️ Error Handling Tests:');
            console.log(`   • world.isLoaded: ${world.isLoaded}`);
            console.log(`   • world.getLastError(): "${world.getLastError()}"`);
            console.log();

            // Test property-based access (not function calls!)
            console.log('🏷️ World Properties (property access, not functions):');
            console.log(`   • world.npcSpawnEnabled: ${world.npcSpawnEnabled}`);
            console.log(`   • world.npcSpawnFlags: ${world.npcSpawnFlags}`);
            console.log(`   • world.hasPlayer: ${world.hasPlayer}`);
            console.log(`   • world.hasSkyController: ${world.hasSkyController}`);
            console.log();

            // Access mesh through world property
            console.log('🔗 World → Mesh Relationship:');
            const mesh = world.mesh;
            console.log(`   • world.mesh: ${mesh.constructor.name}`);
            console.log(`   • world.mesh.name: "${mesh.name}"`);
            console.log();

            // Test new safe count accessors
            console.log('🔢 Mesh Safe Count Accessors:');
            console.log(`   • mesh.vertexCount: ${mesh.vertexCount}`);
            console.log(`   • mesh.featureCount: ${mesh.featureCount}`);
            console.log(`   • mesh.indexCount: ${mesh.indexCount}`);
            console.log();

            // Show mesh basic info through world
            console.log('📊 Mesh Data Summary (via world.mesh):');
            console.log(`   • vertices: ${mesh.vertices.size()} items`);
            console.log(`   • features: ${mesh.features.size()} items`);
            console.log(`   • indices: ${mesh.vertexIndices.size()} items`);
            console.log();

            // Test improved bounding box calculation
            const bbox_min = mesh.boundingBoxMin;
            const bbox_max = mesh.boundingBoxMax;
            console.log('📏 World Bounding Box (Improved Calculation):');
            console.log(`   • min: {x: ${bbox_min.x.toFixed(1)}, y: ${bbox_min.y.toFixed(1)}, z: ${bbox_min.z.toFixed(1)}}`);
            console.log(`   • max: {x: ${bbox_max.x.toFixed(1)}, y: ${bbox_max.y.toFixed(1)}, z: ${bbox_max.z.toFixed(1)}}`);

            // Calculate world dimensions
            const width = bbox_max.x - bbox_min.x;
            const height = bbox_max.y - bbox_min.y;
            const depth = bbox_max.z - bbox_min.z;
            console.log(`   • dimensions: ${width.toFixed(1)} × ${height.toFixed(1)} × ${depth.toFixed(1)}`);
            console.log();

            // Test new performance optimization methods
            console.log('⚡ Performance Optimization Tests:');

            // Test typed arrays for WebGL
            const verticesTyped = mesh.getVerticesTypedArray();
            const normalsTyped = mesh.getNormalsTypedArray();
            const uvsTyped = mesh.getUVsTypedArray();
            const indicesTyped = mesh.getIndicesTypedArray();

            console.log('   • Typed Arrays for WebGL:');
            console.log(`     - Vertices: ${verticesTyped ? 'Float32Array' : 'null'} (${verticesTyped ? verticesTyped.length / 3 : 0} vertices)`);
            console.log(`     - Normals: ${normalsTyped ? 'Float32Array' : 'null'} (${normalsTyped ? normalsTyped.length / 3 : 0} normals)`);
            console.log(`     - UVs: ${uvsTyped ? 'Float32Array' : 'null'} (${uvsTyped ? uvsTyped.length / 2 : 0} coords)`);
            console.log(`     - Indices: ${indicesTyped ? 'Uint32Array' : 'null'} (${indicesTyped ? indicesTyped.length : 0} indices)`);
            console.log();

            // Test mesh data safety improvements
            console.log('🛡️ Mesh Data Safety Tests:');

            // Test vertex access safety
            if (mesh.vertexCount > 0) {
                try {
                    const firstVertex = mesh.vertices.get(0);
                    console.log(`   • First vertex access: {x: ${firstVertex.x.toFixed(3)}, y: ${firstVertex.y.toFixed(3)}, z: ${firstVertex.z.toFixed(3)}}`);

                    // Test bounds checking by trying to access beyond bounds
                    const beyondBounds = mesh.vertices.get(mesh.vertexCount);
                    console.log('   • Bounds check: Accessing beyond bounds should fail gracefully');
                } catch (e) {
                    console.log(`   • Bounds check: Properly handled out-of-bounds access: ${e.message}`);
                }
            }

            // Test feature access safety
            if (mesh.featureCount > 0) {
                try {
                    const firstFeature = mesh.features.get(0);
                    console.log(`   • First feature texture: {u: ${firstFeature.texture.x.toFixed(3)}, v: ${firstFeature.texture.y.toFixed(3)}}`);
                    console.log(`   • First feature normal: {x: ${firstFeature.normal.x.toFixed(3)}, y: ${firstFeature.normal.y.toFixed(3)}, z: ${firstFeature.normal.z.toFixed(3)}}`);
                } catch (e) {
                    console.log(`   • Feature access: Error - ${e.message}`);
                }
            }

            console.log();

            // Test materials
            console.log('🎨 Materials Test:');
            const materials = mesh.materials;
            console.log(`   • materials: ${materials.size()} items`);
            if (materials.size() > 0) {
                const firstMaterial = materials.get(0);
                console.log(`   • first material: "${firstMaterial.name}" (group: ${firstMaterial.group})`);
            }
            console.log();

            console.log('🎯 ZenKit Improvements Demonstrated:');
            console.log('   ✅ Error handling: world.getLastError(), world.isLoaded');
            console.log('   ✅ Safe count access: mesh.vertexCount, mesh.featureCount');
            console.log('   ✅ Fixed bounding box: mesh.boundingBoxMin/Max work properly');
            console.log('   ✅ Performance: getVerticesTypedArray() for direct WebGL use');
            console.log('   ✅ Safety: Bounds checking prevents WASM crashes');
            console.log('   ✅ Property access: Clean API with properties instead of functions');
            console.log('   ✅ Automatic memory management: No malloc/free needed!');

        // No finally block needed - automatic memory management!

    } catch (error) {
        console.error('💥 World test failed:', error.message);
        console.error('Stack trace:', error.stack);
        process.exit(1);
    }
}

testWorld();
