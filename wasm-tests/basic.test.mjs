/**
 * Basic ZenKit WASM Tests
 *
 * Tests that don't require external files and can run in any environment.
 * These tests verify the core WASM module functionality.
 */

describe('ZenKit Basic Functionality', () => {
    let zenkit;

    beforeAll(async () => {
        // Load ZenKit WASM module for tests
        zenkit = await setupZenKit();
        expect(zenkit).toBeDefined();
    }, 30000);

    describe('Core Module Tests', () => {
        test('should export all expected functions', () => {
            expect(typeof zenkit.getZenKitVersion).toBe('function');
            expect(typeof zenkit.getLibraryInfo).toBe('function');
            expect(typeof zenkit.createWorld).toBe('function');
            expect(typeof zenkit._malloc).toBe('function');
            expect(typeof zenkit._free).toBe('function');
        });

        test('should have WASM memory interface', () => {
            expect(zenkit.HEAPU8).toBeDefined();
            expect(ArrayBuffer.isView(zenkit.HEAPU8)).toBe(true);
            expect(zenkit.HEAPU8.BYTES_PER_ELEMENT).toBe(1);
        });

        test('should provide version info', () => {
            const version = zenkit.getZenKitVersion();
            expect(version).toMatch(/^\d+\.\d+\.\d+$/);
        });

        test('should provide library info', () => {
            const info = zenkit.getLibraryInfo();
            expect(info).toHaveProperty('version');
            expect(info).toHaveProperty('buildType');
            expect(info).toHaveProperty('hasMmap');
            expect(info).toHaveProperty('debugBuild');
        });
    });

    describe('World Creation Tests', () => {
        test('should create world without errors', () => {
            const world = zenkit.createWorld();
            expect(world).toBeDefined();
            expect(typeof world).toBe('object');
        });

        test('should create multiple worlds independently', () => {
            const world1 = zenkit.createWorld();
            const world2 = zenkit.createWorld();
            const world3 = zenkit.createWorld();

            expect(world1).not.toBe(world2);
            expect(world2).not.toBe(world3);
            expect(world1).not.toBe(world3);
        });

        test('should have world properties', () => {
            const world = zenkit.createWorld();

            // Test that all expected properties exist
            const expectedProperties = [
                'npcSpawnEnabled',
                'npcSpawnFlags',
                'hasPlayer',
                'hasSkyController',
                'mesh',
                'load',
                'loadWithVersion'
            ];

            expectedProperties.forEach(prop => {
                expect(world).toHaveProperty(prop);
            });
        });
    });

    describe('Memory Management Tests', () => {
        test('should allocate memory successfully', () => {
            const sizes = [1, 100, 1000, 10000];

            sizes.forEach(size => {
                const ptr = zenkit._malloc(size);
                expect(typeof ptr).toBe('number');
                expect(ptr).toBeGreaterThan(0);

                // Clean up
                zenkit._free(ptr);
            });
        });

        test('should read and write to allocated memory', () => {
            const size = 1024;
            const ptr = zenkit._malloc(size);

            try {
                // Write some data
                const testData = new Uint8Array(size);
                for (let i = 0; i < size; i++) {
                    testData[i] = i % 256;
                }

                zenkit.HEAPU8.set(testData, ptr);

                // Read it back
                const readData = zenkit.HEAPU8.slice(ptr, ptr + size);

                // Verify
                for (let i = 0; i < size; i++) {
                    expect(readData[i]).toBe(i % 256);
                }
            } finally {
                zenkit._free(ptr);
            }
        });

        test('should handle zero-sized allocations', () => {
            const ptr = zenkit._malloc(0);
            expect(typeof ptr).toBe('number');

            if (ptr !== 0) {
                zenkit._free(ptr);
            }
        });
    });

    describe('Error Handling Tests', () => {
        test('should handle invalid world loading gracefully', () => {
            const world = zenkit.createWorld();

            // Real WASM module throws exceptions for invalid data
            expect(() => {
                world.load(0, 0);
            }).toThrow();
        });

        test('should handle invalid memory access gracefully', () => {
            const world = zenkit.createWorld();

            // Allocate some memory
            const ptr = zenkit._malloc(100);

            try {
                // Real WASM module throws exceptions for invalid memory access
                expect(() => {
                    world.load(ptr, 1000); // Try to read more than allocated
                }).toThrow();
            } finally {
                zenkit._free(ptr);
            }
        });
    });

    describe('Type System Tests', () => {
        test('should handle different data types in memory', () => {
            const size = 1024;
            const ptr = zenkit._malloc(size);

            try {
                // Test different views of the same memory
                const uint8View = new Uint8Array(zenkit.HEAPU8.buffer, ptr, size);
                const uint32View = new Uint32Array(zenkit.HEAPU8.buffer, ptr, size / 4);
                const float32View = new Float32Array(zenkit.HEAPU8.buffer, ptr, size / 4);

                // Write data using different types
                uint8View[0] = 255;
                uint8View[1] = 255;
                uint8View[2] = 255;
                uint8View[3] = 255;

                // Read as uint32
                expect(uint32View[0]).toBe(4294967295);

                // Write as float
                float32View[0] = 3.14159;
                expect(float32View[0]).toBeCloseTo(3.14159, 5);

            } finally {
                zenkit._free(ptr);
            }
        });
    });

    describe('Performance Tests', () => {
        test('should allocate memory quickly', () => {
            const iterations = 100;
            const allocations = [];

            const startTime = Date.now();

            // Allocate many small blocks
            for (let i = 0; i < iterations; i++) {
                const ptr = zenkit._malloc(64);
                allocations.push(ptr);
            }

            // Free them all
            allocations.forEach(ptr => zenkit._free(ptr));

            const endTime = Date.now();
            const totalTime = endTime - startTime;

            // Should complete in reasonable time (allow 1 second for 100 allocations)
            expect(totalTime).toBeLessThan(1000);
        });

        test('should create worlds quickly', () => {
            const iterations = 100;
            const worlds = [];

            const startTime = Date.now();

            // Create many worlds
            for (let i = 0; i < iterations; i++) {
                worlds.push(zenkit.createWorld());
            }

            const endTime = Date.now();
            const totalTime = endTime - startTime;

            // Should complete in reasonable time
            expect(totalTime).toBeLessThan(500);

            // Verify all worlds are valid
            worlds.forEach(world => {
                expect(world).toBeDefined();
                expect(world).toHaveProperty('mesh');
            });
        });
    });
});

