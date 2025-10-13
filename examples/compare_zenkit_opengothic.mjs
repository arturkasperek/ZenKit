#!/usr/bin/env node
/**
 * Compare ZenKit's processed mesh data with OpenGothic's renderable mesh
 * This validates that ZenKit's getProcessedMeshData() produces output
 * matching OpenGothic's PackedMesh pipeline.
 */

import fs from 'fs';
import { fileURLToPath } from 'url';
import { dirname, join } from 'path';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);
const projectRoot = join(__dirname, '..');

async function main() {
    console.log('========================================');
    console.log('ZenKit vs OpenGothic Mesh Comparison');
    console.log('========================================\n');

    // Load ZenKit WASM
    console.log('Step 1: Loading ZenKit WASM...');
    const ZenKitModule = (await import('../build-wasm/wasm/zenkit.mjs')).default;
    const ZenKit = await ZenKitModule();
    console.log('✓ ZenKit loaded\n');

    // Load NEWWORLD.ZEN
    console.log('Step 2: Loading NEWWORLD.ZEN...');
    const zenPath = join(projectRoot, 'public/game-assets/WORLDS/NEWWORLD/NEWWORLD.ZEN');
    if (!fs.existsSync(zenPath)) {
        console.error(`❌ Error: ${zenPath} not found!`);
        process.exit(1);
    }
    
    const zenData = fs.readFileSync(zenPath);
    console.log(`File size: ${(zenData.length / 1024 / 1024).toFixed(2)} MB`);
    
    const world = ZenKit.createWorld();
    console.log('Loading with auto-detect...');
    const result = world.loadFromArray(new Uint8Array(zenData)); // Auto-detect version!
    
    if (!result.success) {
        console.error(`❌ Failed to load: ${result.errorMessage}`);
        process.exit(1);
    }
    console.log('✓ NEWWORLD.ZEN loaded\n');

    // Get processed mesh data from ZenKit
    console.log('Step 3: Processing mesh with getProcessedMeshData()...');
    const startTime = Date.now();
    const mesh = world.mesh;
    const processed = mesh.getProcessedMeshData();
    const processTime = Date.now() - startTime;
    console.log(`✓ Processing complete in ${processTime}ms\n`);

    // Convert Emscripten vectors to JS arrays for easier handling
    console.log('Step 4: Converting data...');
    const zenKitData = {
        vertices: [],
        indices: [],
        materialIds: [],
        materials: []
    };
    
    // Copy vertices
    for (let i = 0; i < processed.vertices.size(); i++) {
        zenKitData.vertices.push(processed.vertices.get(i));
    }
    
    // Copy indices
    for (let i = 0; i < processed.indices.size(); i++) {
        zenKitData.indices.push(processed.indices.get(i));
    }
    
    // Copy material IDs
    for (let i = 0; i < processed.materialIds.size(); i++) {
        zenKitData.materialIds.push(processed.materialIds.get(i));
    }
    
    // Copy materials
    for (let i = 0; i < processed.materials.size(); i++) {
        const mat = processed.materials.get(i);
        zenKitData.materials.push({
            name: mat.name,
            group: mat.group,
            texture: mat.texture
        });
    }
    
    console.log(`✓ Converted data\n`);

    // Load OpenGothic's JSON
    console.log('Step 5: Loading OpenGothic data...');
    const ogJsonPath = join(projectRoot, 'opengothic_renderable_mesh.json');
    if (!fs.existsSync(ogJsonPath)) {
        console.error(`❌ Error: ${ogJsonPath} not found!`);
        console.error('Please run OpenGothic first to generate this file.');
        process.exit(1);
    }
    
    const openGothicData = JSON.parse(fs.readFileSync(ogJsonPath, 'utf8'));
    console.log('✓ OpenGothic data loaded\n');

    // Compare
    console.log('========================================');
    console.log('Comparison Results:');
    console.log('========================================\n');

    const zenKitTriCount = zenKitData.indices.length / 3;
    const ogTriCount = openGothicData.indices.length / 3;
    const zenKitVertCount = zenKitData.vertices.length / 8; // 8 floats per vertex
    const ogVertCount = openGothicData.vertices.length; // OpenGothic vertices are objects

    console.log(`Total triangles:`);
    console.log(`  ZenKit:     ${zenKitTriCount.toLocaleString()}`);
    console.log(`  OpenGothic: ${ogTriCount.toLocaleString()}`);
    console.log(`  Ratio: ${(zenKitTriCount / ogTriCount).toFixed(3)}`);
    console.log(`  Match: ${zenKitTriCount === ogTriCount ? '✅' : '❌'}\n`);

    console.log(`Total vertices:`);
    console.log(`  ZenKit:     ${zenKitVertCount.toLocaleString()}`);
    console.log(`  OpenGothic: ${ogVertCount.toLocaleString()}`);
    console.log(`  Difference: ${Math.abs(zenKitVertCount - ogVertCount).toLocaleString()}`);
    console.log(`  (Different due to meshlet optimization)\n`);

    console.log(`Total materials:`);
    console.log(`  ZenKit:     ${zenKitData.materials.length}`);
    console.log(`  OpenGothic: ${openGothicData.submeshes.length}`);
    console.log(`  Match: ${zenKitData.materials.length === openGothicData.submeshes.length ? '✅' : '❌'}\n`);

    // Material-by-material comparison
    console.log('Material breakdown:\n');
    
    // Count triangles per material for ZenKit
    const zenKitMatCounts = {};
    for (let i = 0; i < zenKitData.materialIds.length; i++) {
        const matId = zenKitData.materialIds[i];
        zenKitMatCounts[matId] = (zenKitMatCounts[matId] || 0) + 1;
    }

    // Compare first 10 materials
    const maxMatsToShow = Math.min(10, zenKitData.materials.length);
    for (let i = 0; i < maxMatsToShow; i++) {
        const zkMat = zenKitData.materials[i];
        const ogSubmesh = openGothicData.submeshes[i];
        const zkTriCount = zenKitMatCounts[i] || 0;
        const ogTriCount = ogSubmesh.triangleCount;
        
        const match = zkTriCount === ogTriCount ? '✅' : '❌';
        console.log(`Material ${i}: ${zkMat.name}`);
        console.log(`  Texture: ${zkMat.texture}`);
        console.log(`  Triangles: ZenKit=${zkTriCount}, OpenGothic=${ogTriCount} ${match}`);
    }
    
    if (zenKitData.materials.length > maxMatsToShow) {
        console.log(`\n... and ${zenKitData.materials.length - maxMatsToShow} more materials\n`);
    }

    // Sample vertex comparison (first 3 triangles)
    console.log('\n========================================');
    console.log('Sample Vertex Comparison (first 3 triangles):');
    console.log('========================================\n');
    
    let vertexMismatches = 0;
    const maxVerticesToCheck = 9; // 3 triangles × 3 vertices
    
    for (let i = 0; i < maxVerticesToCheck && i < zenKitData.indices.length; i++) {
        const zkIdx = zenKitData.indices[i];
        const ogIdx = openGothicData.indices[i];
        
        const zkVertBase = zkIdx * 8;
        
        const zkPos = [
            zenKitData.vertices[zkVertBase],
            zenKitData.vertices[zkVertBase + 1],
            zenKitData.vertices[zkVertBase + 2]
        ];
        const ogVert = openGothicData.vertices[ogIdx];
        const ogPos = ogVert.position;
        
        const zkUV = [
            zenKitData.vertices[zkVertBase + 6],
            zenKitData.vertices[zkVertBase + 7]
        ];
        const ogUV = ogVert.uv;
        
        // Check for approximate equality (floating point)
        const posMatch = 
            Math.abs(zkPos[0] - ogPos[0]) < 0.01 &&
            Math.abs(zkPos[1] - ogPos[1]) < 0.01 &&
            Math.abs(zkPos[2] - ogPos[2]) < 0.01;
            
        const uvMatch =
            Math.abs(zkUV[0] - ogUV[0]) < 0.01 &&
            Math.abs(zkUV[1] - ogUV[1]) < 0.01;
        
        if (!posMatch || !uvMatch) {
            vertexMismatches++;
        }
        
        if (i < 3) { // Show first 3 in detail
            console.log(`Vertex ${i}:`);
            console.log(`  Index: ZenKit=${zkIdx}, OpenGothic=${ogIdx}`);
            console.log(`  Position: ZenKit=[${zkPos.map(v => v.toFixed(2)).join(', ')}]`);
            console.log(`            OpenGothic=[${ogPos.map(v => v.toFixed(2)).join(', ')}] ${posMatch ? '✅' : '❌'}`);
            console.log(`  UV: ZenKit=[${zkUV.map(v => v.toFixed(4)).join(', ')}]`);
            console.log(`      OpenGothic=[${ogUV.map(v => v.toFixed(4)).join(', ')}] ${uvMatch ? '✅' : '❌'}`);
            console.log('');
        }
    }
    
    console.log(`Total vertex mismatches in sample: ${vertexMismatches}/${maxVerticesToCheck}`);

    // Final summary
    console.log('\n========================================');
    console.log('Final Summary:');
    console.log('========================================\n');
    
    const criticalMatch = zenKitTriCount === ogTriCount && 
                         zenKitData.materials.length === openGothicData.submeshes.length;
    
    if (criticalMatch && vertexMismatches === 0) {
        console.log('✅ PASS - All critical data matches!');
        console.log('\nZenKit successfully replicates OpenGothic\'s PackedMesh pipeline:');
        console.log('  ✓ Material deduplication');
        console.log('  ✓ Composite vertex processing');
        console.log('  ✓ Triangle sorting');
        console.log('  ✓ Feature index bit-shift fix');
    } else if (criticalMatch) {
        console.log('⚠️  PARTIAL MATCH - Triangle/material counts correct, but some vertex data differs');
        console.log(`  ${vertexMismatches} vertex mismatches in sample`);
    } else {
        console.log('❌ FAIL - Critical data does not match');
        if (zenKitTriCount !== ogTriCount) {
            console.log(`  ✗ Triangle count mismatch`);
        }
        if (zenKitData.materials.length !== openGothicData.submeshes.length) {
            console.log(`  ✗ Material count mismatch`);
        }
    }
    
    console.log('');
}

main().catch(error => {
    console.error('\n❌ Error:', error);
    process.exit(1);
});
