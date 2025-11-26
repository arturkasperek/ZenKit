#!/usr/bin/env node

/**
 * Standalone Node.js test script for Daedalus VM item visual lookup and function calls
 * 
 * This script tests if we can load GOTHIC.DAT and:
 * - Access item instance properties (specifically the visual property of items)
 * - Call internal VM functions from the DAT file
 * 
 * Usage:
 *   node test-daedalus-item-visual.mjs [path/to/GOTHIC.DAT] [itemInstanceName] [functionName]
 * 
 * Examples:
 *   node test-daedalus-item-visual.mjs public/game-assets/SCRIPTS/_COMPILED/GOTHIC.DAT ItRw_Addon_FireArrow
 *   node test-daedalus-item-visual.mjs public/game-assets/SCRIPTS/_COMPILED/GOTHIC.DAT ItRw_Addon_FireArrow dia_xardas_hello_info
 */

import ZenKitModule from './build-wasm/wasm/zenkit.mjs';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const sleep = (ms) => new Promise(resolve => setTimeout(resolve, ms));

async function testDaedalusItemVisual() {
    try {
        // Get command line arguments
        const scriptPath = process.argv[2] || path.join(__dirname, '..', 'public', 'game-assets', 'SCRIPTS', '_COMPILED', 'GOTHIC.DAT');
        const itemInstanceName = process.argv[3] || 'ItRw_Addon_FireArrow';
        const functionName = process.argv[4] || 'dia_xardas_hello_info';

        // Check if script file exists
        if (!fs.existsSync(scriptPath)) {
            console.error(`❌ Script file not found: ${scriptPath}`);
            process.exit(1);
        }

        // Load ZenKit WASM module
        const ZenKit = await ZenKitModule();

        // Load script file
        const scriptBuffer = fs.readFileSync(scriptPath);
        const uint8Array = new Uint8Array(scriptBuffer);
        
        const script = ZenKit.createDaedalusScript();
        const loadResult = script.loadFromArray(uint8Array);
        
        if (!loadResult.success) {
            console.error(`❌ Failed to load GOTHIC.DAT: ${loadResult.error_message}`);
            process.exit(1);
        }

        // Create VM
        const vm = ZenKit.createDaedalusVm(script);

        // Check if instance exists
        if (!vm.hasSymbol(itemInstanceName)) {
            console.error(`❌ Instance '${itemInstanceName}' not found in script`);
            process.exit(1);
        }

        // Try to get visual property
        let visualValue = '';
        try {
            visualValue = vm.getSymbolString('VISUAL', itemInstanceName) || vm.getSymbolString('visual', itemInstanceName);
        } catch (error) {
            console.error(`❌ Failed to retrieve visual property:`, error);
            process.exit(1);
        }

        if (visualValue && visualValue !== '') {
            console.log(`✅ Item '${itemInstanceName}' visual: "${visualValue}"`);
        }

        // Register external handlers
        vm.registerExternal('AI_Output', async (npc0, npc1, text) => {
            const npc0Name = npc0.name || `NPC[${npc0.symbol_index}]`;
            const npc1Name = npc1.name || `NPC[${npc1.symbol_index}]`;
            console.log(`💬 ${npc0Name} -> ${npc1Name}: "${text}"`);
            await sleep(1000);
        });

        vm.registerExternal('INFO_CLEARCHOICES', (infoInstance) => {
            console.log(`🗑️  Clearing choices for info instance ${infoInstance}`);
        });

        vm.registerExternal('INFO_ADDCHOICE', (infoInstance, text, func) => {
            console.log(`➕ Adding choice: "${text}" (func: ${func})`);
        });

        // Set up global context variables (self and other)
        const selfNpcName = 'NONE_100_XARDAS';
        const otherNpcName = 'PC_HERO';
        
        if (vm.hasSymbol(selfNpcName)) {
            vm.setGlobalSelf(selfNpcName);
        }
        if (vm.hasSymbol(otherNpcName)) {
            vm.setGlobalOther(otherNpcName);
        }

        // Call VM function
        if (!vm.hasSymbol(functionName)) {
            console.error(`❌ Function '${functionName}' not found`);
            process.exit(1);
        }
        
        try {
            const callResult = vm.callFunction(functionName, []);
            if (!callResult.success) {
                console.error(`❌ Function call failed: ${callResult.errorMessage}`);
            }
        } catch (e) {
            const errorMsg = typeof e === 'number' 
                ? `VM error (code: ${e})` 
                : (e instanceof Error ? e.message : String(e));
            console.error(`❌ Exception: ${errorMsg}`);
        }

    } catch (error) {
        console.error('\n❌ Fatal error:', error);
        if (error instanceof Error) {
            console.error('Stack:', error.stack);
        }
        process.exit(1);
    }
}

// Run the test
testDaedalusItemVisual();

