#!/usr/bin/env node

/**
 * Simple test to verify ZenKit WASM loads without errors
 */

async function testBasicWASM() {
    console.log('🧪 Testing ZenKit WASM basic initialization...\n');
    
    try {
        // Import and initialize ZenKit
        const ZenKitModule = (await import('./public/zenkit.mjs')).default;
        console.log('✅ ZenKit module imported');
        
        const ZenKit = await ZenKitModule();
        console.log('✅ ZenKit initialized without errors!');
        console.log(`✅ Version: ${ZenKit.getZenKitVersion()}`);
        
        // Check that key types exist
        const types = [
            'VectorVec3',
            'VectorMeshWedge',
            'VectorMeshTriangle',
            'VectorSubMesh',
            'VectorSoftSkinWeightEntry',
            'VectorVectorSoftSkinWeightEntry'
        ];
        
        console.log('\n🔍 Checking critical vector types:');
        let allFound = true;
        for (const typeName of types) {
            const exists = typeof ZenKit[typeName] !== 'undefined';
            if (exists) {
                console.log(`   ✅ ${typeName}`);
            } else {
                console.log(`   ❌ ${typeName} - MISSING`);
                allFound = false;
            }
        }
        
        if (!allFound) {
            throw new Error('Some required types are missing');
        }
        
        console.log('\n✅ All required types are registered');
        console.log('🎉 WASM is ready for use!');
        console.log('\n📋 Summary:');
        console.log('   - No duplicate registration errors');
        console.log('   - All vector types available');
        console.log('   - Ready for CPU skinning implementation');
        
        process.exit(0);
        
    } catch (error) {
        console.error('\n❌ Test failed:', error.message);
        if (error.stack) {
            console.error('\nStack trace:');
            console.error(error.stack);
        }
        process.exit(1);
    }
}

testBasicWASM();
