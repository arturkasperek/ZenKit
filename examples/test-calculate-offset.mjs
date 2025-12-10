import ZenKitModule from '../build-wasm/wasm/zenkit.mjs';
import { readFileSync } from 'fs';

const ZenKit = await ZenKitModule();
console.log(`Testing calculateGeometryOffset\n${'='.repeat(70)}\n`);

// Load hierarchy (HUMANS.MDH)
const mdhPath = `/Users/artur/dev/gothic/ZenKit/public/game-assets/ANIMS/_COMPILED/HUMANS.MDH`;
const mdhBuffer = readFileSync(mdhPath);
const mdhArray = new Uint8Array(mdhBuffer);

const hierarchyLoader = ZenKit.createModelHierarchyLoader();
const mdhResult = hierarchyLoader.loadFromArray(mdhArray);

if (!mdhResult.success) {
    console.log('❌ Failed to load hierarchy');
    process.exit(1);
}

const hierarchy = hierarchyLoader.getHierarchy();
console.log(`✅ Loaded hierarchy: ${hierarchy.nodes.size()} nodes\n`);

const testModels = [
    { name: 'HUM_BODY_NAKED0', expected: 'very small or ~0' },
    { name: 'HUM_BODY_BABE0', expected: '~14 in Z axis' },
    { name: 'ARMOR_RAVEN_ADDON', expected: '~16 in Z axis' },
    { name: 'ARMOR_MIL_L', expected: '~16 in Z axis' }
];

for (const test of testModels) {
    try {
        const path = `/Users/artur/dev/gothic/ZenKit/public/game-assets/ANIMS/_COMPILED/${test.name}.MDM`;
        const buffer = readFileSync(path);
        const uint8Array = new Uint8Array(buffer);
        
        const meshLoader = ZenKit.createModelMeshLoader();
        const loadResult = meshLoader.loadFromArray(uint8Array);
        
        if (!loadResult.success) {
            console.log(`${test.name}: ❌ Failed to load\n`);
            continue;
        }
        
        const mesh = meshLoader.getMesh();
        const model = ZenKit.createModel();
        model.setMesh(mesh);
        
        const softSkinMeshes = model.getSoftSkinMeshes();
        if (softSkinMeshes.size() === 0) {
            console.log(`${test.name}: ⚠️ No meshes\n`);
            continue;
        }
        
        // Calculate offset for first soft-skin mesh
        const ssm = softSkinMeshes.get(0);
        const offset = model.calculateGeometryOffset(ssm, hierarchy);
        
        console.log(`📦 ${test.name}:`);
        console.log(`   Expected: ${test.expected}`);
        console.log(`   Calculated offset: (${offset.x.toFixed(3)}, ${offset.y.toFixed(3)}, ${offset.z.toFixed(3)})`);
        console.log(`   ✅ Use this in viewer: geometry.translate(${offset.x.toFixed(2)}, ${offset.y.toFixed(2)}, ${offset.z.toFixed(2)})`);
        console.log();
        
    } catch (err) {
        console.log(`${test.name}: ❌ ${err.message}\n`);
    }
}

console.log('\n💡 EXPLANATION:');
console.log('   The offset is: (ideal_position_from_bone_local) - (mesh.positions)');
console.log('   This is the exact compensation needed for Three.js GPU skinning!');

process.exit(0);
