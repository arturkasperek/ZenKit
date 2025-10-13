#!/usr/bin/env node
import fs from 'fs';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';

const __dirname = dirname(fileURLToPath(import.meta.url));

async function test() {
    const ZenKitModule = (await import('./build-wasm/wasm/zenkit.mjs')).default;
    const ZenKit = await ZenKitModule();
    
    const zenPath = join(__dirname, 'public/game-assets/WORLDS/NEWWORLD/NEWWORLD.ZEN');
    const zenData = fs.readFileSync(zenPath);
    
    console.log(`File size: ${(zenData.length / 1024 / 1024).toFixed(2)} MB\n`);
    
    const world = ZenKit.createWorld();
    
    // Test with both Gothic versions
    console.log('Testing with AUTO-DETECT (default):');
    const result1 = world.loadFromArray(new Uint8Array(zenData));
    if (result1.success) {
        const mesh1 = world.mesh;
        console.log(`  Material indices: ${mesh1.getPolygonMaterialIndicesTypedArray().length}`);
        console.log(`  Vertex indices: ${mesh1.getIndicesTypedArray().length}`);
        console.log(`  Expected triangles: ${mesh1.getPolygonMaterialIndicesTypedArray().length}`);
    }
    
    // Create new world for second test
    const world2 = ZenKit.createWorld();
    console.log('\nTesting with explicit GOTHIC_2:');
    const result2 = world2.loadFromArray(new Uint8Array(zenData), 2);
    if (result2.success) {
        const mesh2 = world2.mesh;
        console.log(`  Material indices: ${mesh2.getPolygonMaterialIndicesTypedArray().length}`);
        console.log(`  Vertex indices: ${mesh2.getIndicesTypedArray().length}`);
        console.log(`  Expected triangles: ${mesh2.getPolygonMaterialIndicesTypedArray().length}`);
    }
    
    // Try with Gothic 1
    const world3 = ZenKit.createWorld();
    console.log('\nTesting with explicit GOTHIC_1:');
    const result3 = world3.loadFromArray(new Uint8Array(zenData), 1);
    if (result3.success) {
        const mesh3 = world3.mesh;
        console.log(`  Material indices: ${mesh3.getPolygonMaterialIndicesTypedArray().length}`);
        console.log(`  Vertex indices: ${mesh3.getIndicesTypedArray().length}`);
        console.log(`  Expected triangles: ${mesh3.getPolygonMaterialIndicesTypedArray().length}`);
    } else {
        console.log(`  Failed: ${result3.errorMessage}`);
    }
}

test().catch(console.error);
