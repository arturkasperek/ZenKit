#!/usr/bin/env node

/**
 * Test script to verify we can access mesh data through the WASM bindings
 */

import { readFile } from 'fs/promises';
import { fileURLToPath } from 'url';
import { dirname, join } from 'path';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

async function testMeshAccess() {
    console.log('🧪 Testing mesh data access through WASM...\n');
    
    try {
        // Import and initialize ZenKit
        const ZenKitModule = (await import('./public/zenkit.mjs')).default;
        const ZenKit = await ZenKitModule();
        console.log('✅ ZenKit initialized:', ZenKit.getZenKitVersion());
        
        // Try to load an MDS file
        const mdsPath = join(__dirname, 'public/game-assets/ANIMS/HumanS.mds');
        console.log(`\n📂 Loading MDS: ${mdsPath}`);
        
        try {
            const buffer = await readFile(mdsPath);
            const uint8Array = new Uint8Array(buffer);
            
            // Create ModelScript and load
            const mds = ZenKit.createModelScript();
            const loadResult = mds.loadFromArray(uint8Array);
            
            if (!loadResult.success) {
                throw new Error(loadResult.errorMessage || 'Unknown loading error');
            }
            
            const skelName = mds.skeleton ? mds.skeleton.name : '(no skeleton)';
            console.log(`✅ MDS loaded: ${skelName}`);
            
            // Access model hierarchy
            const model = mds.modelHierarchy;
            if (!model) {
                throw new Error('No model hierarchy in MDS');
            }
            console.log(`✅ Model hierarchy exists`);
            
            // Try to access soft skin meshes
            const softSkinMeshes = model.meshes;
            const meshCount = softSkinMeshes.size();
            console.log(`✅ Soft skin mesh count: ${meshCount}`);
            
            if (meshCount > 0) {
                const firstMesh = softSkinMeshes.get(0);
                console.log('✅ Got first soft skin mesh');
                
                // Access the MultiResolutionMesh inside
                const mrMesh = firstMesh.mesh;
                console.log('✅ Got MultiResolutionMesh');
                
                // Try to access positions (VectorVec3)
                const positions = mrMesh.positions;
                console.log(`✅ Positions vector: ${positions.size()} vertices`);
                
                if (positions.size() > 0) {
                    const firstPos = positions.get(0);
                    console.log(`   First vertex: (${firstPos.x.toFixed(2)}, ${firstPos.y.toFixed(2)}, ${firstPos.z.toFixed(2)})`);
                }
                
                // Try to access normals
                const normals = mrMesh.normals;
                console.log(`✅ Normals vector: ${normals.size()} normals`);
                
                // Try to access subMeshes
                const subMeshes = mrMesh.subMeshes;
                console.log(`✅ SubMeshes vector: ${subMeshes.size()} submeshes`);
                
                if (subMeshes.size() > 0) {
                    const firstSubMesh = subMeshes.get(0);
                    console.log('✅ Got first SubMesh');
                    
                    // Access material
                    const mat = firstSubMesh.mat;
                    console.log(`✅ Material texture: ${mat.texture}`);
                    
                    // Access triangles
                    const triangles = firstSubMesh.triangles;
                    console.log(`✅ Triangles: ${triangles.size()}`);
                    
                    // Access wedges
                    const wedges = firstSubMesh.wedges;
                    console.log(`✅ Wedges: ${wedges.size()}`);
                    
                    if (wedges.size() > 0) {
                        const firstWedge = wedges.get(0);
                        console.log(`   First wedge: index=${firstWedge.index}, uv=(${firstWedge.texture.x.toFixed(3)}, ${firstWedge.texture.y.toFixed(3)})`);
                    }
                }
                
                // Try to access weights
                const weights = firstMesh.weights;
                console.log(`✅ Weights: ${weights.size()} vertices`);
                
                if (weights.size() > 0) {
                    const firstVertexWeights = weights.get(0);
                    console.log(`   First vertex has ${firstVertexWeights.size()} weight entries`);
                    
                    if (firstVertexWeights.size() > 0) {
                        const firstWeight = firstVertexWeights.get(0);
                        console.log(`      Weight: bone=${firstWeight.nodeIndex}, weight=${firstWeight.weight.toFixed(3)}`);
                    }
                }
            }
            
            console.log('\n✅ All mesh data access tests passed!');
            console.log('🎉 WASM bindings are working correctly!');
            
        } catch (e) {
            console.error('❌ Failed to load/parse MDS:', e.message);
            throw e;
        }
        
        process.exit(0);
        
    } catch (error) {
        console.error('\n❌ Test failed:', error.message);
        console.error(error.stack);
        process.exit(1);
    }
}

testMeshAccess();
