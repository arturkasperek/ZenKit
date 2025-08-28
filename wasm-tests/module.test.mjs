/**
 * Basic ZenKit WASM Module Tests
 *
 * Tests the core functionality of loading and initializing the ZenKit WASM module.
 */

describe('ZenKit WASM Module', () => {
    let zenkit;

    beforeAll(async () => {
        // Load ZenKit WASM module for tests
        zenkit = await setupZenKit();
        expect(zenkit).toBeDefined();
    }, 30000);

    describe('Module Initialization', () => {
        test('should load ZenKit WASM module successfully', () => {
            expect(zenkit).toBeDefined();
            expect(typeof zenkit).toBe('object');
        });

        test('should have getZenKitVersion function', () => {
            expect(typeof zenkit.getZenKitVersion).toBe('function');
        });

        test('should return valid version string', () => {
            const version = zenkit.getZenKitVersion();
            expect(typeof version).toBe('string');
            expect(version.length).toBeGreaterThan(0);
            expect(version).toMatch(/^\d+\.\d+\.\d+$/); // Should match semantic versioning
        });

        test('should have getLibraryInfo function', () => {
            expect(typeof zenkit.getLibraryInfo).toBe('function');
        });

        test('should return library info object', () => {
            const info = zenkit.getLibraryInfo();
            expect(info).toBeDefined();
            expect(typeof info).toBe('object');
            expect(info).toHaveProperty('version');
            expect(info).toHaveProperty('buildType');
            expect(info).toHaveProperty('hasMmap');
            expect(info).toHaveProperty('debugBuild');
        });

        test('should have createWorld function', () => {
            expect(typeof zenkit.createWorld).toBe('function');
        });
    });

    describe('World Creation', () => {
        test('should create world instance successfully', () => {
            const world = zenkit.createWorld();
            expect(world).toBeDefined();
            expect(typeof world).toBe('object');
        });

        test('should have world loading methods', () => {
            const world = zenkit.createWorld();
            expect(typeof world.load).toBe('function');
            expect(typeof world.loadWithVersion).toBe('function');
        });

        test('should have world properties', () => {
            const world = zenkit.createWorld();
            expect(world).toHaveProperty('npcSpawnEnabled');
            expect(world).toHaveProperty('npcSpawnFlags');
            expect(world).toHaveProperty('hasPlayer');
            expect(world).toHaveProperty('hasSkyController');
            expect(world).toHaveProperty('mesh');
        });
    });

    describe('WASM Memory Management', () => {
        test('should have WASM memory management functions', () => {
            expect(typeof zenkit._malloc).toBe('function');
            expect(typeof zenkit._free).toBe('function');
            expect(zenkit.HEAPU8).toBeDefined();
        });

        test('should be able to allocate and free memory', () => {
            const size = 1024;
            const ptr = zenkit._malloc(size);
            expect(typeof ptr).toBe('number');
            expect(ptr).toBeGreaterThan(0);

            // Free the memory
            zenkit._free(ptr);
        });
    });
});

