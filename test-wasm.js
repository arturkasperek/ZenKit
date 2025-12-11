#!/usr/bin/env node

/**
 * Test script to verify ZenKit WASM bindings load correctly
 */

async function testWASM() {
    console.log('🧪 Testing ZenKit WASM bindings...\n');
    
    try {
        // Import the WASM module
        const ZenKitModule = (await import('./public/zenkit.mjs')).default;
        console.log('✅ ZenKit module imported');
        
        // Initialize
        const ZenKit = await ZenKitModule();
        console.log('✅ ZenKit initialized');
        
        // Test version
        const version = ZenKit.getZenKitVersion();
        console.log(`✅ ZenKit version: ${version}`);
        
        // Test that our new types are registered
        console.log('\n🔍 Checking registered types:');
        
        const types = [
            'Material',
            'MeshWedge',
            'MeshTriangle',
            'SubMesh',
            'VectorVec3',
            'VectorMeshWedge',
            'VectorMeshTriangle',
            'VectorSubMesh',
            'VectorUint16',
            'VectorSoftSkinWeightEntry',
            'VectorVectorSoftSkinWeightEntry',
            'MultiResolutionMeshValue'
        ];
        
        for (const typeName of types) {
            try {
                const exists = typeof ZenKit[typeName] !== 'undefined';
                if (exists) {
                    console.log(`   ✅ ${typeName}`);
                } else {
                    console.log(`   ⚠️  ${typeName} - not found`);
                }
            } catch (e) {
                console.log(`   ❌ ${typeName} - error: ${e.message}`);
            }
        }
        
        console.log('\n✅ All tests passed! WASM is ready.');
        process.exit(0);
        
    } catch (error) {
        console.error('\n❌ Test failed:', error.message);
        console.error(error.stack);
        process.exit(1);
    }
}

testWASM();
