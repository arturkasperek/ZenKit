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
    
    const world = ZenKit.createWorld();
    const result = world.loadFromArray(new Uint8Array(zenData));
    
    const mesh = world.mesh;
    
    console.log('RAW MESH DATA FROM ZENKIT:');
    console.log(`  Vertices: ${mesh.vertexCount}`);
    console.log(`  Features: ${mesh.featureCount}`);
    console.log(`  Indices: ${mesh.indexCount}`);
    console.log(`  Raw Triangles: ${mesh.indexCount / 3}`);
    
    const polyMatIdx = mesh.getPolygonMaterialIndicesTypedArray();
    console.log(`  Material indices array: ${polyMatIdx.length}`);
    console.log(`  Expected triangles from material array: ${polyMatIdx.length}`);
    
    // Get processed data
    const processed = mesh.getProcessedMeshData();
    console.log('\nPROCESSED MESH DATA:');
    console.log(`  Processed triangles: ${processed.materialIds.size()}`);
    console.log(`  Processed vertices: ${processed.vertices.size() / 8}`);
    console.log(`  Deduplicated materials: ${processed.materials.size()}`);
    
    console.log('\nOPENGOTHIC DATA:');
    const og = JSON.parse(fs.readFileSync('opengothic_renderable_mesh.json', 'utf8'));
    console.log(`  Triangles: ${og.indices.length / 3}`);
    console.log(`  Vertices: ${og.vertices.length}`);
    console.log(`  Materials (submeshes): ${og.submeshes.length}`);
    
    console.log('\n📊 COMPARISON:');
    console.log(`  Raw ZenKit triangles: ${mesh.indexCount / 3}`);
    console.log(`  Processed ZenKit triangles: ${processed.materialIds.size()}`);
    console.log(`  OpenGothic triangles: ${og.indices.length / 3}`);
    console.log(`  Difference: ${og.indices.length / 3 - processed.materialIds.size()}`);
}

test().catch(console.error);
