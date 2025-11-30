#!/usr/bin/env node

/**
 * Simple Node.js test script for MDS (Model Script) file loading
 * 
 * This script tests if we can load a .MDS file and access its properties.
 * 
 * Usage:
 *   node test-mds.mjs [path/to/file.mds]
 * 
 * Examples:
 *   node test-mds.mjs public/game-assets/anims/HumanS.mds
 */

import ZenKitModule from './build-wasm/wasm/zenkit.mjs';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

async function testMds() {
    try {
        // Get command line arguments
        const mdsPath = process.argv[2] || path.join(__dirname, 'public', 'game-assets', 'ANIMS', 'HumanS.mds');

        // Check if MDS file exists
        if (!fs.existsSync(mdsPath)) {
            console.error(`❌ MDS file not found: ${mdsPath}`);
            console.error(`   Please provide a valid path to an MDS file.`);
            process.exit(1);
        }

        // Load ZenKit WASM module
        console.log(`📦 Loading ZenKit WASM module...`);
        const ZenKit = await ZenKitModule();
        console.log(`✅ ZenKit ${ZenKit.getZenKitVersion()} loaded`);

        // Load MDS file
        console.log(`\n📂 Loading MDS file: ${mdsPath}`);
        const mdsBuffer = fs.readFileSync(mdsPath);
        const mdsArray = new Uint8Array(mdsBuffer);
        
        const mds = ZenKit.createModelScript();
        
        console.log(`   Loading file data (${mdsArray.length} bytes)...`);
        const loadResult = mds.loadFromArray(mdsArray);
        
        if (!loadResult || !loadResult.success) {
            const errorMsg = loadResult?.errorMessage || mds.getLastError() || 'Unknown error';
            console.error(`❌ Failed to load MDS file`);
            console.error(`   Error: ${errorMsg}`);
            process.exit(1);
        }

        console.log(`✅ MDS file loaded successfully\n`);

        // Display skeleton information
        console.log(`📋 Skeleton Information:`);
        const skeletonName = mds.getSkeletonName();
        const skeletonMeshDisabled = mds.isSkeletonMeshDisabled();
        console.log(`   Name: ${skeletonName || '(none)'}`);
        console.log(`   Mesh Disabled: ${skeletonMeshDisabled ? 'Yes' : 'No'}`);

        // Display meshes
        const meshCount = mds.getMeshCount();
        console.log(`\n📋 Meshes (${meshCount}):`);
        if (meshCount === 0) {
            console.log(`   No meshes defined`);
        } else {
            for (let i = 0; i < meshCount; i++) {
                const meshName = mds.getMeshName(i);
                console.log(`   [${i}] ${meshName}`);
            }
        }

        // Display disabled animations
        const disabledCount = mds.getDisabledAnimationCount();
        console.log(`\n📋 Disabled Animations (${disabledCount}):`);
        if (disabledCount === 0) {
            console.log(`   No disabled animations`);
        } else {
            for (let i = 0; i < disabledCount; i++) {
                const animName = mds.getDisabledAnimationName(i);
                console.log(`   [${i}] ${animName}`);
            }
        }

        // Display animations
        const animCount = mds.getAnimationCount();
        console.log(`\n📋 Animations (${animCount}):`);
        if (animCount === 0) {
            console.log(`   No animations defined`);
        } else {
            // Show first 10 animations with details
            const showCount = Math.min(10, animCount);
            for (let i = 0; i < showCount; i++) {
                const name = mds.getAnimationName(i);
                const layer = mds.getAnimationLayer(i);
                const next = mds.getAnimationNext(i);
                const blendIn = mds.getAnimationBlendIn(i);
                const blendOut = mds.getAnimationBlendOut(i);
                const flags = mds.getAnimationFlags(i);
                const model = mds.getAnimationModel(i);
                const firstFrame = mds.getAnimationFirstFrame(i);
                const lastFrame = mds.getAnimationLastFrame(i);
                const fps = mds.getAnimationFps(i);
                const speed = mds.getAnimationSpeed(i);
                
                console.log(`   [${i}] ${name}`);
                console.log(`       Layer: ${layer}, Next: ${next || '(none)'}`);
                console.log(`       Blend: In=${blendIn.toFixed(2)}, Out=${blendOut.toFixed(2)}`);
                console.log(`       Flags: 0x${flags.toString(16)}, Model: ${model || '(none)'}`);
                console.log(`       Frames: ${firstFrame}-${lastFrame}, FPS: ${fps.toFixed(2)}, Speed: ${speed.toFixed(2)}`);
            }
            
            if (animCount > showCount) {
                console.log(`   ... and ${animCount - showCount} more animations`);
            }
        }

        // Summary statistics
        console.log(`\n📊 Summary:`);
        console.log(`   Total meshes: ${meshCount}`);
        console.log(`   Total disabled animations: ${disabledCount}`);
        console.log(`   Total animations: ${animCount}`);

        console.log(`\n✅ MDS test completed successfully`);

    } catch (error) {
        console.error('\n❌ Fatal error:', error);
        if (error instanceof Error) {
            console.error('Stack:', error.stack);
        }
        process.exit(1);
    }
}

// Run the test
testMds();
