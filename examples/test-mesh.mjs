#!/usr/bin/env node

// Test Mesh data access and properties
import ZenKitModule from './zenkit.mjs';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

async function testMesh() {
    try {
        console.log('🔺 Testing Mesh Data Access...\n');

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
            // Load world and get mesh
            const world = ZenKit.createWorld();
            const result = world.load(wasmMemory, uint8Array.length);
            
            if (!result.success) {
                console.log(`❌ Failed to load: ${result.errorMessage}`);
                return;
            }
            
            console.log('✅ World loaded, accessing mesh...\n');
            
            // Get mesh through property access
            const mesh = world.mesh;
            console.log('🏷️ Mesh Properties (data, not counts):');
            console.log(`   • mesh.name: "${mesh.name}"`);
            
            // Access data vectors directly as properties
            const vertices = mesh.vertices;
            const features = mesh.features;
            const indices = mesh.vertexIndices;
            const normals = mesh.normals;
            const textureCoords = mesh.textureCoords;
            const lightValues = mesh.lightValues;
            
            console.log('\n📊 Mesh Data Vectors:');
            console.log(`   • mesh.vertices: ${vertices.constructor.name} with ${vertices.size()} items`);
            console.log(`   • mesh.features: ${features.constructor.name} with ${features.size()} items`);
            console.log(`   • mesh.vertexIndices: ${indices.constructor.name} with ${indices.size()} items`);
            console.log(`   • mesh.normals: ${normals.constructor.name} with ${normals.size()} items`);
            console.log(`   • mesh.textureCoords: ${textureCoords.constructor.name} with ${textureCoords.size()} items`);
            console.log(`   • mesh.lightValues: ${lightValues.constructor.name} with ${lightValues.size()} items`);
            
            // Sample vertex data
            console.log('\n🎯 Sample Vertex Data:');
            if (vertices.size() > 0) {
                for (let i = 0; i < Math.min(3, vertices.size()); i++) {
                    const vertex = vertices.get(i);
                    console.log(`   [${i}] vertex: {x: ${vertex.x.toFixed(2)}, y: ${vertex.y.toFixed(2)}, z: ${vertex.z.toFixed(2)}}`);
                }
            }
            
            // Sample feature data (structured)
            console.log('\n🌟 Sample Feature Data (structured):');
            if (features.size() > 0) {
                for (let i = 0; i < Math.min(3, features.size()); i++) {
                    const feature = features.get(i);
                    console.log(`   [${i}] feature:`);
                    console.log(`       • normal: {x: ${feature.normal.x.toFixed(3)}, y: ${feature.normal.y.toFixed(3)}, z: ${feature.normal.z.toFixed(3)}}`);
                    console.log(`       • texture: {x: ${feature.texture.x.toFixed(3)}, y: ${feature.texture.y.toFixed(3)}}`);
                    console.log(`       • light: ${feature.light}`);
                }
            }
            
            // Individual component vectors
            console.log('\n🧩 Individual Component Access:');
            if (normals.size() > 0) {
                const firstNormal = normals.get(0);
                console.log(`   • First normal: {x: ${firstNormal.x.toFixed(3)}, y: ${firstNormal.y.toFixed(3)}, z: ${firstNormal.z.toFixed(3)}}`);
            }
            if (textureCoords.size() > 0) {
                const firstUV = textureCoords.get(0);
                console.log(`   • First texture coord: {x: ${firstUV.x.toFixed(3)}, y: ${firstUV.y.toFixed(3)}}`);
            }
            if (lightValues.size() > 0) {
                console.log(`   • First light value: ${lightValues.get(0)}`);
            }
            
            // Demonstrate JavaScript array conversion if needed
            console.log('\n🔄 Converting to JavaScript Arrays (if needed):');
            const jsVertices = [];
            for (let i = 0; i < Math.min(3, vertices.size()); i++) {
                const vertex = vertices.get(i);
                jsVertices.push({x: vertex.x, y: vertex.y, z: vertex.z});
            }
            console.log(`   • First 3 vertices as JS array:`, jsVertices);
            
            // Bounding box
            console.log('\n📦 Bounding Box:');
            const bbox_min = mesh.boundingBoxMin;
            const bbox_max = mesh.boundingBoxMax;
            console.log(`   • min: {x: ${bbox_min.x.toFixed(1)}, y: ${bbox_min.y.toFixed(1)}, z: ${bbox_min.z.toFixed(1)}}`);
            console.log(`   • max: {x: ${bbox_max.x.toFixed(1)}, y: ${bbox_max.y.toFixed(1)}, z: ${bbox_max.z.toFixed(1)}}`);
            
            console.log('\n🎯 Key Benefits Demonstrated:');
            console.log('   ✅ Properties not functions: mesh.vertices (not mesh.getVertices())');
            console.log('   ✅ JavaScript calculates sizes: mesh.vertices.size()');
            console.log('   ✅ Structured data preserved: feature.normal.x, feature.texture.y');
            console.log('   ✅ Efficient Emscripten vectors: direct C++ memory access');
            console.log('   ✅ Multiple access patterns: full features OR individual components');
            
        } finally {
            ZenKit._free(wasmMemory);
        }
        
    } catch (error) {
        console.error('💥 Mesh test failed:', error.message);
        process.exit(1);
    }
}

testMesh();
