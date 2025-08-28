/**
 * ZenKit World Loading and Property Tests
 *
 * Tests the functionality of loading ZEN files and accessing world properties.
 */

// Simplified for Jest compatibility
import fs from 'fs';

describe('ZenKit World', () => {
    let zenkit;

    beforeAll(async () => {
        // Load ZenKit WASM module for tests
        zenkit = await setupZenKit();
        expect(zenkit).toBeDefined();
    }, 30000);

    describe('World Loading', () => {
        test('should create world successfully', () => {
            const world = zenkit.createWorld();
            expect(world).toBeDefined();
            expect(typeof world.load).toBe('function');
            expect(typeof world.loadWithVersion).toBe('function');
        });

        test('should handle invalid data gracefully', () => {
            const world = zenkit.createWorld();

            // Real WASM throws exceptions for invalid data
            expect(() => {
                world.load(0, 0);
            }).toThrow();
        });

        test('should handle oversized data', () => {
            const world = zenkit.createWorld();

            // Allocate small memory but try to load large data
            const ptr = zenkit._malloc(50);

            try {
                // Real WASM throws exceptions for invalid memory access
                expect(() => {
                    world.load(ptr, 1000);
                }).toThrow();
            } finally {
                zenkit._free(ptr);
            }
        });
    });

    describe('World Properties', () => {
        let world;

        beforeEach(() => {
            world = zenkit.createWorld();
        });

        test('should have npcSpawnEnabled property', () => {
            expect(typeof world.npcSpawnEnabled).toBe('boolean');
        });

        test('should have npcSpawnFlags property', () => {
            expect(typeof world.npcSpawnFlags).toBe('number');
            expect(world.npcSpawnFlags).toBeGreaterThanOrEqual(0);
        });

        test('should have hasPlayer property', () => {
            expect(typeof world.hasPlayer).toBe('boolean');
        });

        test('should have hasSkyController property', () => {
            expect(typeof world.hasSkyController).toBe('boolean');
        });

        test('should have mesh property', () => {
            expect(world.mesh).toBeDefined();
            expect(typeof world.mesh).toBe('object');
        });

        test('should provide consistent property values', () => {
            // Test that properties return consistent values when accessed multiple times
            const npcSpawnEnabled1 = world.npcSpawnEnabled;
            const npcSpawnEnabled2 = world.npcSpawnEnabled;
            expect(npcSpawnEnabled1).toBe(npcSpawnEnabled2);

            const npcSpawnFlags1 = world.npcSpawnFlags;
            const npcSpawnFlags2 = world.npcSpawnFlags;
            expect(npcSpawnFlags1).toBe(npcSpawnFlags2);
        });
    });

    describe('World Mesh Access', () => {
        let world;

        beforeEach(() => {
            world = zenkit.createWorld();
        });

        test('should access mesh through world property', () => {
            const mesh = world.mesh;
            expect(mesh).toBeDefined();
            expect(typeof mesh).toBe('object');

            // Mesh should have basic properties
            expect(mesh).toHaveProperty('name');
            expect(mesh).toHaveProperty('vertices');
            expect(mesh).toHaveProperty('boundingBoxMin');
            expect(mesh).toHaveProperty('boundingBoxMax');
        });

        test('should have valid mesh name', () => {
            const mesh = world.mesh;
            expect(typeof mesh.name).toBe('string');
        });

        test('should have vertices data structure', () => {
            const mesh = world.mesh;
            expect(mesh.vertices).toBeDefined();
            expect(typeof mesh.vertices).toBe('object');
            expect(typeof mesh.vertices.size).toBe('function');
        });

        test('should handle mesh access on unloaded world', () => {
            // Accessing mesh on unloaded world should not crash
            expect(() => {
                const mesh = world.mesh;
                expect(mesh).toBeDefined();
            }).not.toThrow();
        });

        test('should handle mesh property access on unloaded world', () => {
            const mesh = world.mesh;

            // These should not crash even if world is not loaded
            expect(() => {
                const name = mesh.name;
                expect(typeof name).toBe('string');
            }).not.toThrow();

            expect(() => {
                const vertices = mesh.vertices;
                expect(vertices).toBeDefined();
            }).not.toThrow();
        });
    });

    describe('Concurrent World Access', () => {
        test('should handle multiple world instances', () => {
            const world1 = zenkit.createWorld();
            const world2 = zenkit.createWorld();
            const world3 = zenkit.createWorld();

            expect(world1).not.toBe(world2);
            expect(world2).not.toBe(world3);
            expect(world1).not.toBe(world3);

            // Each should have its own properties
            expect(world1).toHaveProperty('mesh');
            expect(world2).toHaveProperty('mesh');
            expect(world3).toHaveProperty('mesh');
        });
    });

    describe('World Resource Management', () => {
        test('should clean up resources after errors', () => {
            const world = zenkit.createWorld();

            // Try loading invalid data multiple times - real WASM throws exceptions
            for (let i = 0; i < 5; i++) {
                expect(() => {
                    world.load(0, 0);
                }).toThrow();
            }

            // World should still be usable
            expect(world).toHaveProperty('mesh');
            expect(world).toHaveProperty('load');
        });

        test('should handle memory cleanup after failed loads', () => {
            const world = zenkit.createWorld();

            // Create data that will trigger a parsing error
            const testData = new Uint8Array([0x00, 0x00, 0x00, 0x00]); // Corrupted header
            const ptr = zenkit._malloc(testData.length);
            zenkit.HEAPU8.set(testData, ptr);

            try {
                // Try loading (should fail due to corrupted header) - real WASM throws exceptions
                expect(() => {
                    world.load(ptr, testData.length);
                }).toThrow();

                // Memory should still be valid and freeable (but real WASM may throw on double free)
                // We'll skip the double free test since we've already tested it above
                expect(ptr).toBeGreaterThan(0);
            } finally {
                // Only free once
                zenkit._free(ptr);
            }
        });
    });
});

describe('Archive File Loading (ascii.zen)', () => {
    test('should successfully load ascii.zen file into WASM memory', async () => {
        // First initialize ZenKit
        const zenkit = await setupZenKit();

        const zenFilePath = getTestDataPath('ascii.zen');
        const zenData = loadFileIntoWasm(zenFilePath, zenkit);

        try {
            // Test that file was loaded successfully
            expect(zenData.pointer).toBeGreaterThan(0);
            expect(zenData.size).toBeGreaterThan(0);
            expect(typeof zenData.cleanup).toBe('function');

            // Verify file content by checking file size matches expected
            const expectedSize = fs.statSync(zenFilePath).size;
            expect(zenData.size).toBe(expectedSize);

            console.log(`Successfully loaded ascii.zen: ${zenData.size} bytes at pointer ${zenData.pointer}`);

        } finally {
            zenData.cleanup();
        }
    });

    test('should handle file loading errors gracefully', async () => {
        // First initialize ZenKit
        const zenkit = await setupZenKit();

        // Test with non-existent file
        const nonExistentPath = getTestDataPath('nonexistent.zen');

        expect(() => {
            loadFileIntoWasm(nonExistentPath, zenkit);
        }).toThrow('Test file not found');

        // Test with valid file path
        const zenFilePath = getTestDataPath('ascii.zen');
        const zenData = loadFileIntoWasm(zenFilePath, zenkit);

        try {
            // Verify memory allocation worked
            expect(zenData.pointer).toBeDefined();
            expect(zenData.pointer).not.toBeNull();
            expect(zenData.pointer).toBeGreaterThan(0);

        } finally {
            zenData.cleanup();
        }
    });

    test('should demonstrate ZenKit WASM module functionality', async () => {
        // First initialize ZenKit
        const zenkit = await setupZenKit();

        // Test that ZenKit module has expected functions
        expect(zenkit).toBeDefined();
        expect(typeof zenkit.getZenKitVersion).toBe('function');

        // Test version function
        const version = zenkit.getZenKitVersion();
        expect(typeof version).toBe('string');
        expect(version.length).toBeGreaterThan(0);

        console.log(`ZenKit WASM version: ${version}`);

        // Test library info
        const libInfo = zenkit.getLibraryInfo();
        expect(libInfo).toBeDefined();
        expect(libInfo).toHaveProperty('version');
        expect(libInfo).toHaveProperty('buildType');

        console.log(`ZenKit library info:`, libInfo);
    });
});