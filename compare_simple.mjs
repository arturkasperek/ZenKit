#!/usr/bin/env node
import fs from 'fs';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';

const __dirname = dirname(fileURLToPath(import.meta.url));

async function test() {
    console.log('📊 Simple Triangle Count Comparison\n');
    
    // Load ZenKit
    const ZenKitModule = (await import('./build-wasm/wasm/zenkit.mjs')).default;
    const ZenKit = await ZenKitModule();
    
    const zenPath = join(__dirname, 'public/game-assets/WORLDS/NEWWORLD/NEWWORLD.ZEN');
    const zenData = fs.readFileSync(zenPath);
    
    const world = ZenKit.createWorld();
    world.loadFromArray(new Uint8Array(zenData));
    
    const mesh = world.mesh;
    const processed = mesh.getProcessedMeshData();
    
    console.log('ZenKit (from NEWWORLD.ZEN):');
    console.log(`  Raw material_indices: ${mesh.getPolygonMaterialIndicesTypedArray().length}`);
    console.log(`  Processed triangles: ${processed.materialIds.size()}`);
    console.log(`  Deduplicated materials: ${processed.materials.size()}`);
    
    // Load OpenGothic
    const og = JSON.parse(fs.readFileSync('opengothic_renderable_mesh.json', 'utf8'));
    
    console.log('\nOpenGothic (from opengothic_renderable_mesh.json):');
    console.log(`  Total triangles: ${og.indices.length / 3}`);
    console.log(`  Submeshes: ${og.submeshes.length}`);
    
    console.log('\n❓ Question: Why the difference?');
    console.log(`  Difference: ${og.indices.length / 3 - processed.materialIds.size()} triangles`);
    console.log(`  Ratio: ${((processed.materialIds.size() / (og.indices.length / 3)) * 100).toFixed(1)}%`);
    
    console.log('\n💡 Possible reasons:');
    console.log('  1. OpenGothic merges VOB meshes into world mesh');
    console.log('  2. OpenGothic uses different ZenKit version with more data');
    console.log('  3. OpenGothic JSON was generated from different ZEN file');
    console.log('  4. OpenGothic loads additional geometry we\'re missing');
    
    // Check first few material names
    console.log('\n🔍 First 5 materials comparison:');
    for (let i = 0; i < Math.min(5, processed.materials.size()); i++) {
        const zkMat = processed.materials.get(i);
        const ogMat = og.submeshes[i];
        console.log(`  [${i}] ZK: "${zkMat.name}" | OG: "${ogMat.material_name}"`);
    }
}

test().catch(console.error);
