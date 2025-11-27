#!/usr/bin/env node

/**
 * Simple Node.js test script for waypoint access from world files
 * 
 * This script tests if we can load a .ZEN world file and access waypoints.
 * 
 * Usage:
 *   node test-waypoints.mjs [path/to/world.ZEN]
 * 
 * Examples:
 *   node test-waypoints.mjs public/game-assets/Worlds/NEWWORLD/NEWWORLD.ZEN
 */

import ZenKitModule from './build-wasm/wasm/zenkit.mjs';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

async function testWaypoints() {
    try {
        // Get command line arguments
        const worldPath = process.argv[2] || path.join(__dirname, '..', 'public', 'game-assets', 'Worlds', 'NEWWORLD', 'NEWWORLD.ZEN');

        // Check if world file exists
        if (!fs.existsSync(worldPath)) {
            console.error(`❌ World file not found: ${worldPath}`);
            process.exit(1);
        }

        // Load ZenKit WASM module
        const ZenKit = await ZenKitModule();

        // Load world file
        console.log(`📂 Loading world: ${worldPath}`);
        const worldBuffer = fs.readFileSync(worldPath);
        const worldArray = new Uint8Array(worldBuffer);
        
        const world = ZenKit.createWorld();
        
        // Try loading with auto-detect first, then fallback to specific versions
        let loadResult = null;
        let loaded = false;
        
        // Try auto-detect (version 0)
        try {
            console.log(`   Trying auto-detect version...`);
            loadResult = world.loadFromArray(worldArray, 0);
            if (loadResult && loadResult.success && world.isLoaded) {
                loaded = true;
                console.log(`   ✅ Loaded with auto-detect`);
            }
        } catch (error) {
            // Continue to try other versions
        }
        
        // If auto-detect failed, try Gothic 2
        if (!loaded) {
            try {
                console.log(`   Trying Gothic 2...`);
                const world2 = ZenKit.createWorld();
                loadResult = world2.loadFromArray(worldArray, 2);
                if (loadResult && loadResult.success && world2.isLoaded) {
                    // Replace world with successfully loaded one
                    Object.setPrototypeOf(world, Object.getPrototypeOf(world2));
                    Object.assign(world, world2);
                    loaded = true;
                    console.log(`   ✅ Loaded with Gothic 2`);
                }
            } catch (error) {
                // Continue to try Gothic 1
            }
        }
        
        // If still not loaded, try Gothic 1
        if (!loaded) {
            try {
                console.log(`   Trying Gothic 1...`);
                const world1 = ZenKit.createWorld();
                loadResult = world1.loadFromArray(worldArray, 1);
                if (loadResult && loadResult.success && world1.isLoaded) {
                    // Replace world with successfully loaded one
                    Object.setPrototypeOf(world, Object.getPrototypeOf(world1));
                    Object.assign(world, world1);
                    loaded = true;
                    console.log(`   ✅ Loaded with Gothic 1`);
                }
            } catch (error) {
                // All versions failed
            }
        }
        
        if (!loaded) {
            const errorMsg = loadResult?.errorMessage || world.getLastError() || 'Unknown error';
            console.error(`❌ Failed to load world with any version`);
            console.error(`   Error: ${errorMsg}`);
            process.exit(1);
        }

        console.log(`✅ World loaded successfully`);

        // Get waypoint count
        const waypointCount = world.getWaypointCount();
        console.log(`\n📍 Total waypoints: ${waypointCount}`);

        if (waypointCount === 0) {
            console.warn(`⚠️  No waypoints found in world`);
            process.exit(0);
        }

        // Get first few waypoints
        console.log(`\n📋 First 5 waypoints:`);
        for (let i = 0; i < Math.min(5, waypointCount); i++) {
            const wpResult = world.getWaypoint(i);
            if (wpResult.success) {
                const wp = wpResult.data;
                console.log(`  [${i}] ${wp.name}`);
                console.log(`      Position: (${wp.position.x.toFixed(2)}, ${wp.position.y.toFixed(2)}, ${wp.position.z.toFixed(2)})`);
                console.log(`      Direction: (${wp.direction.x.toFixed(2)}, ${wp.direction.y.toFixed(2)}, ${wp.direction.z.toFixed(2)})`);
                console.log(`      Water depth: ${wp.water_depth}, Under water: ${wp.under_water}, Free point: ${wp.free_point}`);
            } else {
                console.error(`  [${i}] Error: ${wpResult.errorMessage}`);
            }
        }

        // Try to find a specific waypoint by name (common spawn points)
        const testWaypointNames = [
            'NW_CITY_ENTRANCE_01',
            'NW_FARM1_PATH_SPAWN_07',
            'NW_XARDAS_PATH_FARM1_11',
            'START',
            'START_01'
        ];

        console.log(`\n🔍 Searching for specific waypoints:`);
        for (const name of testWaypointNames) {
            const wpResult = world.findWaypointByName(name);
            if (wpResult.success) {
                const wp = wpResult.data;
                console.log(`  ✅ Found: ${wp.name}`);
                console.log(`      Position: (${wp.position.x.toFixed(2)}, ${wp.position.y.toFixed(2)}, ${wp.position.z.toFixed(2)})`);
            } else {
                console.log(`  ❌ Not found: ${name}`);
            }
        }

        // Get waypoint edges (connections)
        const edgeCount = world.getWaypointEdgeCount();
        console.log(`\n🔗 Total waypoint edges (connections): ${edgeCount}`);

        if (edgeCount > 0) {
            console.log(`\n📋 First 5 waypoint edges:`);
            for (let i = 0; i < Math.min(5, edgeCount); i++) {
                const edgeResult = world.getWaypointEdge(i);
                if (edgeResult.success) {
                    const edge = edgeResult.data;
                    const wpA = world.getWaypoint(edge.waypoint_a_index);
                    const wpB = world.getWaypoint(edge.waypoint_b_index);
                    const nameA = wpA.success ? wpA.data.name : `[${edge.waypoint_a_index}]`;
                    const nameB = wpB.success ? wpB.data.name : `[${edge.waypoint_b_index}]`;
                    console.log(`  [${i}] ${nameA} <-> ${nameB}`);
                }
            }
        }

        // Get all waypoints (sample some)
        console.log(`\n📊 Sampling waypoints:`);
        try {
            const allWaypoints = world.getAllWaypoints();
            console.log(`  Total waypoints retrieved: ${allWaypoints.size()}`);
            
            // Show some statistics
            let freePoints = 0;
            let underWater = 0;
            const nameSet = new Set();
            
            for (let i = 0; i < Math.min(100, allWaypoints.size()); i++) {
                const wp = allWaypoints.get(i);
                if (wp.free_point) freePoints++;
                if (wp.under_water) underWater++;
                nameSet.add(wp.name);
            }
            
            console.log(`  Sample stats (first 100):`);
            console.log(`    Free points: ${freePoints}`);
            console.log(`    Under water: ${underWater}`);
            console.log(`    Unique names: ${nameSet.size}`);
        } catch (e) {
            console.warn(`  ⚠️  Could not retrieve all waypoints: ${e.message}`);
        }

        console.log(`\n✅ Waypoint test completed successfully`);

    } catch (error) {
        console.error('\n❌ Fatal error:', error);
        if (error instanceof Error) {
            console.error('Stack:', error.stack);
        }
        process.exit(1);
    }
}

// Run the test
testWaypoints();

