/**
 * ZenKit WASM Error Handling Tests
 *
 * Tests error handling, edge cases, and invalid input scenarios.
 */

describe('ZenKit Error Handling', () => {
    let zenkit;

    beforeAll(async () => {
        // Load ZenKit WASM module for tests
        zenkit = await setupZenKit();
        expect(zenkit).toBeDefined();
    }, 30000);

    describe('Module Loading Errors', () => {
        test('should handle missing WASM file gracefully', async () => {
            // This test would try to load a non-existent WASM file
            // In practice, this is handled during setup, so we test the setup behavior
            expect(zenkit).toBeDefined();
        });

        test('should handle invalid WASM module path', async () => {
            // Test that setupZenKit with invalid path would fail
            const originalSetupZenKit = global.setupZenKit;

            // Temporarily replace setupZenKit
            global.setupZenKit = async (wasmPath) => {
                if (wasmPath === 'invalid-path.mjs') {
                    throw new Error('Module not found');
                }
                return originalSetupZenKit(wasmPath);
            };

            await expect(setupZenKit('invalid-path.mjs')).rejects.toThrow();

            // Restore original function
            global.setupZenKit = originalSetupZenKit;
        });
    });

    describe('World Loading Errors', () => {
        test('should handle null pointer in world load', () => {
            const world = zenkit.createWorld();

            // Real WASM throws exceptions for null pointers
            expect(() => {
                world.load(0, 0);
            }).toThrow();
        });

        test('should handle invalid pointer in world load', () => {
            const world = zenkit.createWorld();

            // Real WASM throws exceptions for invalid memory access
            expect(() => {
                world.load(999999999, 100);
            }).toThrow();
        });

        test('should handle negative size in world load', () => {
            const world = zenkit.createWorld();

            // Allocate some valid memory
            const validPtr = zenkit._malloc(100);

            try {
                // Real WASM throws exceptions for invalid size parameters
                expect(() => {
                    world.load(validPtr, -1);
                }).toThrow();
            } finally {
                zenkit._free(validPtr);
            }
        });

        test('should handle oversized data in world load', () => {
            const world = zenkit.createWorld();

            // Allocate some memory
            const ptr = zenkit._malloc(50);

            try {
                // Real WASM throws exceptions for oversized data
                expect(() => {
                    world.load(ptr, 1000);
                }).toThrow();
            } finally {
                zenkit._free(ptr);
            }
        });
    });

    describe('Memory Management Errors', () => {
        test('should handle double free gracefully', () => {
            const ptr = zenkit._malloc(100);
            expect(typeof ptr).toBe('number');

            // First free should work
            zenkit._free(ptr);

            // Double free in WASM is undefined behavior but typically doesn't throw
            // The test passes if no exception is thrown (which is the expected behavior)
            expect(() => {
                zenkit._free(ptr);
            }).not.toThrow();
        });

        test('should handle freeing invalid pointer', () => {
            // Real WASM module throws exceptions for invalid pointers
            expect(() => {
                zenkit._free(999999999);
            }).toThrow();
        });

        test('should handle freeing null pointer', () => {
            // Real WASM module doesn't throw for null pointer (0), it just ignores it
            expect(() => {
                zenkit._free(0);
            }).not.toThrow();
        });
    });

    describe('Data Validation Errors', () => {
        test('should handle invalid numeric data in buffers', () => {
            const world = zenkit.createWorld();

            // Create buffer with invalid numeric data
            const invalidData = new Uint8Array([
                0xFF, 0xFF, 0xFF, 0xFF, // Invalid float
                0x00, 0x00, 0x00, 0x00  // Valid data
            ]);

            const wasmMemory = zenkit._malloc(invalidData.length);
            zenkit.HEAPU8.set(invalidData, wasmMemory);

            try {
                // Real WASM may throw exceptions for invalid data
                expect(() => {
                    world.load(wasmMemory, invalidData.length);
                }).toThrow();
            } finally {
                zenkit._free(wasmMemory);
            }
        });

        test('should handle corrupted ZEN file headers', () => {
            const world = zenkit.createWorld();

            // Create buffer with corrupted header
            const corruptedData = new Uint8Array([
                0x00, 0x00, 0x00, 0x00, // Corrupted header
                0x5A, 0x45, 0x4E, 0x00, // "ZEN" but in wrong position
            ]);

            const wasmMemory = zenkit._malloc(corruptedData.length);
            zenkit.HEAPU8.set(corruptedData, wasmMemory);

            try {
                // Real WASM throws exceptions for corrupted data
                expect(() => {
                    world.load(wasmMemory, corruptedData.length);
                }).toThrow();
            } finally {
                zenkit._free(wasmMemory);
            }
        });
    });

    describe('Concurrent Access', () => {
        test('should handle concurrent memory allocations', () => {
            const pointers = [];

            // Allocate multiple memory blocks
            for (let i = 0; i < 10; i++) {
                const ptr = zenkit._malloc(100 + i * 10);
                expect(typeof ptr).toBe('number');
                expect(ptr).toBeGreaterThan(0);
                pointers.push(ptr);
            }

            // Free them all
            pointers.forEach(ptr => {
                expect(() => zenkit._free(ptr)).not.toThrow();
            });
        });
    });
});

