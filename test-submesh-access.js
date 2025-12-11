#!/usr/bin/env node

/**
 * Test script to verify SubMesh bindings work correctly
 * Tests with HUMANS.MDH + HUM_BODY_NAKED0.MDM
 */

import { readFile } from 'fs/promises';
import { fileURLToPath } from 'url';
import { dirname, join } from 'path';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

async function testSubMeshAccess() {
    console.log('🧪 Testing SubMesh access through WASM...\n');
    
    try {
        // Import and initialize ZenKit from build output
        const ZenKitModule = (await import('./build-wasm/wasm/zenkit.mjs')).default;
        const ZenKit = await ZenKitModule();
        console.log('✅ ZenKit initialized:', ZenKit.getZenKitVersion());
        
        // Load the MDH file (hierarchy)
        const mdhPath = join(__dirname, 'public/game-assets/ANIMS/_COMPILED/HUMANS.MDH');
        console.log(`\n📂 Loading MDH: ${mdhPath}`);
        
        const mdhBuffer = await readFile(mdhPath);
        const mdhUint8Array = new Uint8Array(mdhBuffer);
        
        const hierarchyLoader = ZenKit.createModelHierarchyLoader();
        const mdhLoadResult = hierarchyLoader.loadFromArray(mdhUint8Array);
        
        if (!mdhLoadResult.success) {
            throw new Error(mdhLoadResult.errorMessage || 'Failed to load MDH');
        }
        
        console.log('✅ MDH loaded successfully');
        
        // Load the MDM file (mesh)
        const mdmPath = join(__dirname, 'public/game-assets/ANIMS/_COMPILED/HUM_BODY_NAKED0.MDM');
        console.log(`📂 Loading MDM: ${mdmPath}`);
        
        const mdmBuffer = await readFile(mdmPath);
        const mdmUint8Array = new Uint8Array(mdmBuffer);
        
        const meshLoader = ZenKit.createModelMeshLoader();
        const mdmLoadResult = meshLoader.loadFromArray(mdmUint8Array);
        
        if (!mdmLoadResult.success) {
            throw new Error(mdmLoadResult.errorMessage || 'Failed to load MDM');
        }
        
        console.log('✅ MDM loaded successfully');
        
        // Combine hierarchy and mesh into a Model
        const model = ZenKit.createModel();
        model.setHierarchy(hierarchyLoader.getHierarchy());
        model.setMesh(meshLoader.getMesh());
        
        console.log('✅ Model created with hierarchy and mesh');
        
        // Access soft skin meshes from the model
        const softSkinMeshes = model.getSoftSkinMeshes();
        const meshCount = softSkinMeshes.size();
        console.log(`✅ Soft skin meshes: ${meshCount}`);
        
        if (meshCount === 0) {
            throw new Error('No soft skin meshes found in model');
        }
        
        // Get first mesh
        const firstMesh = softSkinMeshes.get(0);
        console.log('✅ Got first soft skin mesh');
        
        // Access the MultiResolutionMesh
        const mrMesh = firstMesh.mesh;
        console.log('✅ Got MultiResolutionMesh');
        
        // Access positions
        const positions = mrMesh.positions;
        console.log(`✅ Positions: ${positions.size()} vertices`);
        
        if (positions.size() > 0) {
            const pos0 = positions.get(0);
            console.log(`   First vertex: (${pos0.x.toFixed(3)}, ${pos0.y.toFixed(3)}, ${pos0.z.toFixed(3)})`);
        }
        
        // Access normals
        const normals = mrMesh.normals;
        console.log(`✅ Normals: ${normals.size()} normals`);
        
        // THIS IS THE CRITICAL TEST - Access SubMeshes
        console.log('\n🔍 CRITICAL TEST - Accessing SubMeshes:');
        const subMeshes = mrMesh.subMeshes;
        console.log(`✅ SubMeshes vector: ${subMeshes.size()} submeshes`);
        
        if (subMeshes.size() === 0) {
            throw new Error('No submeshes found');
        }
        
        // Try to GET a SubMesh - this is where the error happens in browser
        console.log('🔍 Attempting to get first SubMesh (this is where browser fails)...');
        try {
            const firstSubMesh = subMeshes.get(0);
            console.log('✅ SUCCESS! Got first SubMesh');
            
            // Access SubMesh fields
            console.log('🔍 Accessing SubMesh.mat (Material)...');
            const mat = firstSubMesh.mat;
            console.log(`✅ Material texture: "${mat.texture}"`);
            
            console.log('🔍 Accessing SubMesh.triangles...');
            const triangles = firstSubMesh.triangles;
            console.log(`✅ Triangles vector: ${triangles.size()}`);
            
            if (triangles.size() > 0) {
                console.log('🔍 Attempting to get first triangle...');
                const tri0 = triangles.get(0);
                console.log('✅ Got first triangle!');
                
                // Access wedge indices using method
                const wedge0 = tri0.getWedge(0);
                const wedge1 = tri0.getWedge(1);
                const wedge2 = tri0.getWedge(2);
                console.log(`   First triangle wedges: [${wedge0}, ${wedge1}, ${wedge2}]`);
            }
            
            console.log('🔍 Accessing SubMesh.wedges...');
            const wedges = firstSubMesh.wedges;
            console.log(`✅ Wedges vector: ${wedges.size()}`);
            
            if (wedges.size() > 0) {
                console.log('🔍 Attempting to get first wedge...');
                const wedge0 = wedges.get(0);
                console.log('✅ Got first wedge!');
                console.log(`   First wedge: index=${wedge0.index}, uv=(${wedge0.texture.x.toFixed(3)}, ${wedge0.texture.y.toFixed(3)})`);
            }
            
            console.log('🔍 Accessing SubMesh.colors...');
            const colors = firstSubMesh.colors;
            console.log(`✅ Colors: ${colors.size()}`);
            
            console.log('🔍 Accessing SubMesh.triangle_plane_indices...');
            const triPlaneIndices = firstSubMesh.trianglePlaneIndices;
            console.log(`✅ Triangle plane indices: ${triPlaneIndices.size()}`);
            
        } catch (e) {
            console.error('❌ FAILED to get SubMesh!');
            console.error('   Error:', e.message);
            console.error('   This is the same error as in the browser!');
            console.error('   The SubMesh binding is NOT working correctly.');
            throw e;
        }
        
        // Access weights
        console.log('\n🔍 Accessing weight data:');
        const weights = firstMesh.weights;
        console.log(`✅ Weights: ${weights.size()} vertices`);
        
        if (weights.size() > 0) {
            const vertWeights = weights.get(0);
            console.log(`   First vertex has ${vertWeights.size()} weight entries`);
            
            if (vertWeights.size() > 0) {
                const w0 = vertWeights.get(0);
                console.log(`      Weight: bone=${w0.nodeIndex}, weight=${w0.weight.toFixed(3)}`);
            }
        }
        
        console.log('\n✅ ALL TESTS PASSED!');
        console.log('🎉 SubMesh bindings are working correctly!');
        console.log('   The browser should also work now after a hard refresh.');
        
        process.exit(0);
        
    } catch (error) {
        console.error('\n❌ TEST FAILED:', error.message);
        console.error('\nStack trace:');
        console.error(error.stack);
        process.exit(1);
    }
}

testSubMeshAccess();
