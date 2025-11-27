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

async function testDaedalusItemVisual() {
    try {
        // Get command line arguments
        const scriptPath = process.argv[2] || path.join(__dirname, '..', 'public', 'game-assets', 'SCRIPTS', '_COMPILED', 'GOTHIC.DAT');
        const itemInstanceName = process.argv[3] || 'ItRw_Addon_FireArrow';
        const functionName = process.argv[4] || 'dia_xardas_hello_info';
        const cutsceneLibPath = process.argv[5] || path.join(__dirname, 'public', 'game-assets', 'SCRIPTS', 'CONTENT', 'CUTSCENE', 'Ou.bin');

        // Check if script file exists
        if (!fs.existsSync(scriptPath)) {
            console.error(`❌ Script file not found: ${scriptPath}`);
            process.exit(1);
        }

        // Load ZenKit WASM module
        const ZenKit = await ZenKitModule();

        // Load cutscene library
        let cutsceneLib = null;
        if (fs.existsSync(cutsceneLibPath)) {
            console.log(`📚 Loading cutscene library: ${cutsceneLibPath}`);
            const cutsceneBuffer = fs.readFileSync(cutsceneLibPath);
            const cutsceneArray = new Uint8Array(cutsceneBuffer);
            cutsceneLib = ZenKit.createCutsceneLibrary();
            const cutsceneLoadResult = cutsceneLib.loadFromArray(cutsceneArray, 2); // Gothic 2
            if (cutsceneLoadResult.success) {
                console.log(`✅ Cutscene library loaded: ${cutsceneLib.blockCount} blocks`);
            } else {
                console.warn(`⚠️  Failed to load cutscene library: ${cutsceneLoadResult.error_message}`);
                cutsceneLib = null;
            }
        } else {
            console.warn(`⚠️  Cutscene library not found: ${cutsceneLibPath}`);
        }

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
            
            // Try to look up dialogue text from cutscene library
            let dialogueText = text;
            if (cutsceneLib) {
                const block = cutsceneLib.getBlockByName(text);
                if (block !== null && block.text) {
                    dialogueText = block.text;
                    const wavName = block.name || text;
                    console.log(`💬 ${npc0Name} -> ${npc1Name}: "${dialogueText}"`);
                    console.log(`   📢 Output unit: "${text}" | WAV: "${wavName}"`);
                } else {
                    console.log(`💬 ${npc0Name} -> ${npc1Name}: "${text}"`);
                    console.log(`   ⚠️  Output unit "${text}" not found in cutscene library`);
                }
            } else {
                console.log(`💬 ${npc0Name} -> ${npc1Name}: "${text}"`);
            }
        });

        vm.registerExternal('INFO_CLEARCHOICES', (infoInstance) => {
            console.log(`🗑️  Clearing choices for info instance ${infoInstance}`);
        });

        vm.registerExternal('INFO_ADDCHOICE', (infoInstance, text, func) => {
            console.log(`➕ Adding choice: "${text}" (func: ${func})`);
        });

        // Register WLD_INSERTNPC to log NPC insertion with details
        vm.registerExternal('WLD_INSERTNPC', (npcInstanceIndex, spawnpoint) => {
            if (npcInstanceIndex <= 0) {
                console.warn(`⚠️  WLD_INSERTNPC: Invalid NPC instance index: ${npcInstanceIndex}`);
                return;
            }
            
            // Initialize the instance if it doesn't exist
            // This will execute the instance definition code and set all properties
            const initResult = vm.initInstanceByIndex(npcInstanceIndex);
            if (!initResult.success) {
                console.warn(`⚠️  WLD_INSERTNPC: Failed to initialize instance ${npcInstanceIndex}: ${initResult.errorMessage}`);
                return;
            }
            
            // Get NPC symbol name from index
            const nameResult = vm.getSymbolNameByIndex(npcInstanceIndex);
            let npcInfo = {
                instanceIndex: npcInstanceIndex,
                spawnpoint: spawnpoint,
            };
            
            if (nameResult && nameResult.success && nameResult.data) {
                npcInfo.symbolName = nameResult.data;
                
                // Get NPC properties using qualified class names
                // Properties are now available after initialization
                const properties = [
                    { qualified: 'C_NPC.name', type: 'string', key: 'name' },
                    { qualified: 'C_NPC.id', type: 'int', key: 'id' },
                    { qualified: 'C_NPC.guild', type: 'int', key: 'guild' },
                    { qualified: 'C_NPC.level', type: 'int', key: 'level' },
                    { qualified: 'C_NPC.attribute[ATR_HITPOINTS]', type: 'int', key: 'hp' },
                    { qualified: 'C_NPC.attribute[ATR_HITPOINTS_MAX]', type: 'int', key: 'hpmax' },
                ];
                
                for (const prop of properties) {
                    try {
                        if (prop.type === 'string') {
                            const value = vm.getSymbolString(prop.qualified, npcInfo.symbolName);
                            if (value && value.trim() !== '') {
                                npcInfo[prop.key] = value;
                            }
                        } else {
                            const value = vm.getSymbolInt(prop.qualified, npcInfo.symbolName);
                            if (value !== undefined && value !== null) {
                                npcInfo[prop.key] = value;
                            }
                        }
                    } catch (e) {
                        // Skip failed property access
                    }
                }
            }
            
            // Format output
            const nameStr = npcInfo.symbolName || `NPC[${npcInstanceIndex}]`;
            const details = [];
            
            if (npcInfo.name && npcInfo.name.trim() !== '') {
                details.push(`Name: "${npcInfo.name}"`);
            }
            if (npcInfo.id !== undefined && npcInfo.id !== null) {
                details.push(`ID: ${npcInfo.id}`);
            }
            if (npcInfo.guild !== undefined && npcInfo.guild !== null) {
                details.push(`Guild: ${npcInfo.guild}`);
            }
            if (npcInfo.level !== undefined && npcInfo.level !== null) {
                details.push(`Level: ${npcInfo.level}`);
            }
            if (npcInfo.hp !== undefined && npcInfo.hpmax !== undefined && 
                (npcInfo.hp !== 0 || npcInfo.hpmax !== 0)) {
                details.push(`HP: ${npcInfo.hp}/${npcInfo.hpmax}`);
            }
            
            const detailsStr = details.length > 0 ? ` (${details.join(', ')})` : '';
            console.log(`👤 WLD_INSERTNPC: ${nameStr} at "${spawnpoint}"${detailsStr}`);
        });

        // Register external functions called during instance initialization
        const emptyExternals = [
            'WLD_INSERTITEM',
            'WLD_SETTIME',
            'WLD_ASSIGNROOMTOGUILD',
            'PLAYVIDEO',
            'CREATEINVITEMS',
            'CREATEINVITEM',
            'MDL_SETVISUAL',
            'MDL_SETVISUALBODY',
            'MDL_SETMODELSCALE',
            'MDL_SETMODELFATNESS',
            'MDL_APPLYOVERLAYMDS',
            'NPC_SETTALENTSKILL',
            'NPC_SETTOFISTMODE',
            'NPC_SETTOFIGHTMODE',
            'EQUIPITEM',
        ];

        emptyExternals.forEach(funcName => {
            if (vm.hasSymbol(funcName)) {
                try {
                    vm.registerExternal(funcName, () => {
                        // Empty implementation
                    });
                } catch (error) {
                    // Function might not be external or already registered, ignore
                }
            }
        });
        
        // Additional externals needed for instance initialization
        const initExternals = [
            'HLP_RANDOM',               // Random number helper (returns int)
            'B_SETATTRIBUTESTOCHAPTER', // Set attributes to chapter (void)
            'B_CREATEAMBIENTINV',       // Create ambient inventory (void)
            'B_SETNPCVISUAL',           // Set NPC visual (void)
            'B_GIVENPCTALENTS',         // Give NPC talents (void)
            'B_SETFIGHTSKILLS',         // Set fight skills (void)
        ];
        
        initExternals.forEach(funcName => {
            if (vm.hasSymbol(funcName)) {
                try {
                    // HLP_RANDOM returns int, others are void
                    if (funcName === 'HLP_RANDOM') {
                        vm.registerExternal(funcName, () => Math.floor(Math.random() * 100));
                    } else {
                        vm.registerExternal(funcName, () => {
                            // Empty implementation for void functions
                        });
                    }
                } catch (error) {
                    // Function might not be external or already registered, ignore
                }
            }
        });

        // Register int-returning externals
        const intExternals = ['NPC_ISDEAD', 'HLP_ISVALIDNPC'];
        intExternals.forEach(funcName => {
            if (vm.hasSymbol(funcName)) {
                try {
                    vm.registerExternal(funcName, () => 0);
                } catch (error) {
                    // Ignore
                }
            }
        });

        // Register instance-returning externals
        const instanceExternals = ['HLP_GETNPC'];
        instanceExternals.forEach(funcName => {
            if (vm.hasSymbol(funcName)) {
                try {
                    vm.registerExternal(funcName, () => ({ symbol_index: -1 }));
                } catch (error) {
                    // Ignore
                }
            }
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

        // Call startup_newworld first
        const startupFunctionName = 'startup_newworld';
        if (vm.hasSymbol(startupFunctionName)) {
            console.log(`\n🚀 Calling startup function: ${startupFunctionName}`);
            try {
                const startupResult = vm.callFunction(startupFunctionName, []);
                if (!startupResult.success) {
                    console.error(`❌ Startup function call failed: ${startupResult.errorMessage}`);
                } else {
                    console.log(`✅ Startup function completed successfully`);
                }
            } catch (e) {
                const errorMsg = typeof e === 'number' 
                    ? `VM error (code: ${e})` 
                    : (e instanceof Error ? e.message : String(e));
                console.error(`❌ Exception calling startup function: ${errorMsg}`);
            }
        } else {
            console.warn(`⚠️  Startup function '${startupFunctionName}' not found`);
        }

        // Call requested VM function (if different from startup)
        if (functionName !== startupFunctionName) {
            if (!vm.hasSymbol(functionName)) {
                console.error(`❌ Function '${functionName}' not found`);
                process.exit(1);
            }
            
            console.log(`\n🔧 Calling function: ${functionName}`);
            try {
                const callResult = vm.callFunction(functionName, []);
                if (!callResult.success) {
                    console.error(`❌ Function call failed: ${callResult.errorMessage}`);
                } else {
                    console.log(`✅ Function call completed successfully`);
                }
            } catch (e) {
                const errorMsg = typeof e === 'number' 
                    ? `VM error (code: ${e})` 
                    : (e instanceof Error ? e.message : String(e));
                console.error(`❌ Exception: ${errorMsg}`);
            }
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

