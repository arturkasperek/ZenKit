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
        console.log('📜 Testing Daedalus VM Item Visual Lookup...\n');

        // Get command line arguments
        const scriptPath = process.argv[2] || path.join(__dirname, '..', 'public', 'game-assets', 'SCRIPTS', '_COMPILED', 'GOTHIC.DAT');
        const itemInstanceName = process.argv[3] || 'ItRw_Addon_FireArrow';
        const functionName = process.argv[4] || 'dia_xardas_hello_info';

        console.log(`📁 Script path: ${scriptPath}`);
        console.log(`🔍 Item instance: ${itemInstanceName}`);
        console.log(`🔧 Function to call: ${functionName}\n`);

        // Check if script file exists
        if (!fs.existsSync(scriptPath)) {
            console.error(`❌ Script file not found: ${scriptPath}`);
            console.error(`\nPlease provide the path to GOTHIC.DAT as the first argument:`);
            console.error(`  node test-daedalus-item-visual.mjs <path/to/GOTHIC.DAT> [itemInstanceName]`);
            process.exit(1);
        }

        // Load ZenKit WASM module
        console.log('🔧 Loading ZenKit WASM module...');
        const ZenKit = await ZenKitModule();
        console.log(`✅ ZenKit Version: ${ZenKit.getZenKitVersion()}\n`);

        // Load script file
        console.log('📖 Loading GOTHIC.DAT script...');
        const scriptBuffer = fs.readFileSync(scriptPath);
        const uint8Array = new Uint8Array(scriptBuffer);
        
        const script = ZenKit.createDaedalusScript();
        const loadResult = script.loadFromArray(uint8Array);
        
        if (!loadResult.success) {
            console.error(`❌ Failed to load GOTHIC.DAT: ${loadResult.error_message}`);
            process.exit(1);
        }
        
        const symbolCount = script.symbolCount;
        console.log(`✅ Script loaded successfully!`);
        console.log(`   - ${symbolCount} symbols loaded\n`);

        // Create VM (this moves the script into the VM, so script becomes empty)
        console.log('🔧 Creating Daedalus VM...');
        const vm = ZenKit.createDaedalusVm(script);
        console.log(`✅ VM created with ${vm.symbolCount} symbols\n`);

        // Check if instance exists
        console.log(`🔍 Checking if instance '${itemInstanceName}' exists...`);
        if (!vm.hasSymbol(itemInstanceName)) {
            console.error(`❌ Instance '${itemInstanceName}' not found in script`);
            console.log(`\nAvailable instances (first 20):`);
            // Note: We can't easily list all symbols without exposing more API
            process.exit(1);
        }
        console.log(`✅ Instance '${itemInstanceName}' found!\n`);

        // Try to get visual property
        console.log(`🎨 Attempting to get visual property...`);
        console.log(`   Trying uppercase 'VISUAL' first...`);
        
        let visualValue = '';
        let errorOccurred = false;
        let errorDetails = null;

        try {
            visualValue = vm.getSymbolString('VISUAL', itemInstanceName);
            if (!visualValue || visualValue === '') {
                console.log(`   Trying lowercase 'visual'...`);
                visualValue = vm.getSymbolString('visual', itemInstanceName);
            }
        } catch (error) {
            errorOccurred = true;
            errorDetails = error;
            console.error(`   ❌ Error occurred:`, error);
            console.error(`   Error type:`, typeof error);
            if (error instanceof Error) {
                console.error(`   Error message:`, error.message);
                console.error(`   Error stack:`, error.stack);
            }
        }

        if (errorOccurred) {
            console.log(`\n❌ Failed to retrieve visual property`);
            console.log(`\nError details:`);
            console.log(`   - Error:`, errorDetails);
            console.log(`   - Type:`, typeof errorDetails);
            
            if (typeof errorDetails === 'number') {
                console.log(`\n⚠️  Got numeric error code: ${errorDetails}`);
                console.log(`   This might indicate an initialization issue.`);
                console.log(`   The instance might need to be initialized, but initialization failed.`);
            }
            
            // Try to get other properties to see if the issue is specific to VISUAL
            console.log(`\n🔍 Testing other properties...`);
            try {
                const name = vm.getSymbolString('NAME', itemInstanceName) || vm.getSymbolString('name', itemInstanceName);
                console.log(`   NAME: ${name || '(empty)'}`);
            } catch (e) {
                console.log(`   NAME: Error - ${e}`);
            }
            
            try {
                const value = vm.getSymbolInt('VALUE', itemInstanceName) || vm.getSymbolInt('value', itemInstanceName);
                console.log(`   VALUE: ${value || 0}`);
            } catch (e) {
                console.log(`   VALUE: Error - ${e}`);
            }
            
            process.exit(1);
        }

        if (visualValue && visualValue !== '') {
            console.log(`\n✅ Success! Item '${itemInstanceName}' visual: "${visualValue}"\n`);
            
            // Try to get other properties
            console.log(`📋 Getting other item properties...`);
            try {
                const name = vm.getSymbolString('NAME', itemInstanceName) || vm.getSymbolString('name', itemInstanceName);
                if (name) console.log(`   - Name: "${name}"`);
            } catch (e) {
                console.log(`   - Name: Error - ${e}`);
            }
            
            try {
                const value = vm.getSymbolInt('VALUE', itemInstanceName) || vm.getSymbolInt('value', itemInstanceName);
                if (value > 0) console.log(`   - Value: ${value}`);
            } catch (e) {
                console.log(`   - Value: Error - ${e}`);
            }
            
            try {
                const damageTotal = vm.getSymbolInt('DAMAGETOTAL', itemInstanceName) || vm.getSymbolInt('damageTotal', itemInstanceName);
                if (damageTotal > 0) console.log(`   - Damage Total: ${damageTotal}`);
            } catch (e) {
                console.log(`   - Damage Total: Error - ${e}`);
            }
        } else {
            console.log(`\n⚠️  Visual property is empty or not found`);
            console.log(`   This might mean:`);
            console.log(`   - The property name is different`);
            console.log(`   - The instance needs initialization`);
            console.log(`   - The property is not set for this instance`);
        }

        // Register AI_Output external handler
        console.log(`\n🔧 Registering AI_Output external handler...`);
        try {
            const registerResult = vm.registerExternal('AI_Output', async (npc0, npc1, text) => {
                const npc0Name = npc0.name || `NPC[${npc0.symbol_index}]`;
                const npc1Name = npc1.name || `NPC[${npc1.symbol_index}]`;
                console.log(`💬 AI_Output: ${npc0Name} -> ${npc1Name}: "${text}"`);
                await sleep(1000);
            });
            
            if (registerResult.success) {
                console.log(`✅ AI_Output handler registered successfully!`);
            } else {
                console.log(`⚠️  Failed to register AI_Output: ${registerResult.errorMessage}`);
            }
        } catch (error) {
            console.error(`❌ Error registering AI_Output:`, error);
        }

        // Set up global context variables (self and other) before calling function
        console.log(`\n🔧 Setting up VM context (self and other)...`);
        try {
            // Try to find NPC instances for self and other
            // For dia_xardas_hello_info, we need NONE_100_XARDAS (Xardas) and PC_HERO (player)
            const selfNpcName = 'NONE_100_XARDAS';
            const otherNpcName = 'PC_HERO';
            
            if (vm.hasSymbol(selfNpcName)) {
                const setSelfResult = vm.setGlobalSelf(selfNpcName);
                if (setSelfResult.success) {
                    console.log(`✅ Set global 'self' to '${selfNpcName}'`);
                } else {
                    console.log(`⚠️  Failed to set global 'self': ${setSelfResult.errorMessage}`);
                }
            } else {
                console.log(`⚠️  NPC '${selfNpcName}' not found, 'self' will be uninitialized`);
            }
            
            if (vm.hasSymbol(otherNpcName)) {
                const setOtherResult = vm.setGlobalOther(otherNpcName);
                if (setOtherResult.success) {
                    console.log(`✅ Set global 'other' to '${otherNpcName}'`);
                } else {
                    console.log(`⚠️  Failed to set global 'other': ${setOtherResult.errorMessage}`);
                }
            } else {
                console.log(`⚠️  NPC '${otherNpcName}' not found, 'other' will be uninitialized`);
            }
        } catch (error) {
            console.error(`⚠️  Error setting up context:`, error);
        }

        // Try to call VM function
        console.log(`\n🔧 Attempting to call VM function '${functionName}'...`);
        try {
            if (!vm.hasSymbol(functionName)) {
                console.log(`⚠️  Function '${functionName}' not found in script`);
                console.log(`   Available functions might be different.`);
            } else {
                console.log(`✅ Function '${functionName}' found!`);
                
                // Call the function using unified callFunction (no parameters)
                const callResult = vm.callFunction(functionName, []);
                
                if (callResult.success) {
                    console.log(`✅ Function '${functionName}' called successfully!`);
                    console.log(`   (Check above for AI_Output messages if the function calls AI_Output)`);
                } else {
                    console.log(`❌ Function call failed: ${callResult.errorMessage}`);
                }
            }
        } catch (error) {
            console.error(`❌ Error calling function:`, error);
            if (error instanceof Error) {
                console.error(`   Error message:`, error.message);
                console.error(`   Error stack:`, error.stack);
            }
        }
        
        console.log(`\n✅ Test completed!`);

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

