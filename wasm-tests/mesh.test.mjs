/**
 * ZenKit Mesh Data Access Tests
 *
 * Tests the functionality of accessing and manipulating mesh data through WASM bindings.
 */

// Simplified path handling for Jest compatibility

describe('ZenKit Mesh', () => {
    let zenkit;

    beforeAll(async () => {
        // Load ZenKit WASM module for tests
        zenkit = await setupZenKit();
        expect(zenkit).toBeDefined();
    }, 30000);

    describe('Mesh Data Access', () => {
        let world;
        let mesh;

        beforeEach(() => {
            world = zenkit.createWorld();
            mesh = world.mesh;
        });

        test('should have mesh name property', () => {
            expect(typeof mesh.name).toBe('string');
            expect(mesh.name.length).toBeGreaterThanOrEqual(0);
        });

        test('should have vertices data structure', () => {
            expect(mesh.vertices).toBeDefined();
            expect(typeof mesh.vertices).toBe('object');
            expect(typeof mesh.vertices.size).toBe('function');
            expect(typeof mesh.vertices.get).toBe('function');
        });

        test('should have features data structure', () => {
            expect(mesh.features).toBeDefined();
            expect(typeof mesh.features).toBe('object');
            expect(typeof mesh.features.size).toBe('function');
            expect(typeof mesh.features.get).toBe('function');
        });

        test('should have vertex indices', () => {
            expect(mesh.vertexIndices).toBeDefined();
            expect(typeof mesh.vertexIndices).toBe('object');
            expect(typeof mesh.vertexIndices.size).toBe('function');
        });

        test('should have normals data', () => {
            expect(mesh.normals).toBeDefined();
            expect(typeof mesh.normals).toBe('object');
            expect(typeof mesh.normals.size).toBe('function');
        });

        test('should have texture coordinates', () => {
            expect(mesh.textureCoords).toBeDefined();
            expect(typeof mesh.textureCoords).toBe('object');
            expect(typeof mesh.textureCoords.size).toBe('function');
        });

        test('should have light values', () => {
            expect(mesh.lightValues).toBeDefined();
            expect(typeof mesh.lightValues).toBe('object');
            expect(typeof mesh.lightValues.size).toBe('function');
        });

        test('should have bounding box properties', () => {
            expect(mesh.boundingBoxMin).toBeDefined();
            expect(mesh.boundingBoxMax).toBeDefined();

            const min = mesh.boundingBoxMin;
            const max = mesh.boundingBoxMax;

            expect(typeof min.x).toBe('number');
            expect(typeof min.y).toBe('number');
            expect(typeof min.z).toBe('number');
            expect(typeof max.x).toBe('number');
            expect(typeof max.y).toBe('number');
            expect(typeof max.z).toBe('number');
        });

        test('should have consistent data sizes', () => {
            const verticesCount = mesh.vertices.size();
            const featuresCount = mesh.features.size();
            const normalsCount = mesh.normals.size();
            const textureCoordsCount = mesh.textureCoords.size();
            const lightValuesCount = mesh.lightValues.size();

            // Features, normals, texture coords, and light values should have same count as vertices
            expect(featuresCount).toBe(verticesCount);
            expect(normalsCount).toBe(verticesCount);
            expect(textureCoordsCount).toBe(verticesCount);
            expect(lightValuesCount).toBe(verticesCount);
        });
    });

    describe('Mesh Data Validation', () => {
        let world;
        let mesh;

        beforeEach(() => {
            world = zenkit.createWorld();
            mesh = world.mesh;
        });

        test('should have valid vertex data', () => {
            const verticesCount = mesh.vertices.size();

            if (verticesCount > 0) {
                // Test first vertex
                const firstVertex = mesh.vertices.get(0);
                expect(firstVertex).toBeDefined();
                expect(typeof firstVertex.x).toBe('number');
                expect(typeof firstVertex.y).toBe('number');
                expect(typeof firstVertex.z).toBe('number');

                // Test that coordinates are finite numbers
                expect(isFinite(firstVertex.x)).toBe(true);
                expect(isFinite(firstVertex.y)).toBe(true);
                expect(isFinite(firstVertex.z)).toBe(true);
            }
        });

        test('should have valid feature data', () => {
            const featuresCount = mesh.features.size();

            if (featuresCount > 0) {
                const firstFeature = mesh.features.get(0);
                expect(firstFeature).toBeDefined();

                // Check normal
                expect(firstFeature.normal).toBeDefined();
                expect(typeof firstFeature.normal.x).toBe('number');
                expect(typeof firstFeature.normal.y).toBe('number');
                expect(typeof firstFeature.normal.z).toBe('number');

                // Check texture coordinates
                expect(firstFeature.texture).toBeDefined();
                expect(typeof firstFeature.texture.x).toBe('number');
                expect(typeof firstFeature.texture.y).toBe('number');

                // Check light value
                expect(typeof firstFeature.light).toBe('number');
                expect(firstFeature.light).toBeGreaterThanOrEqual(0);
            }
        });

        test('should have valid bounding box', () => {
            const min = mesh.boundingBoxMin;
            const max = mesh.boundingBoxMax;

            // Min should be less than or equal to max for all axes
            expect(min.x).toBeLessThanOrEqual(max.x);
            expect(min.y).toBeLessThanOrEqual(max.y);
            expect(min.z).toBeLessThanOrEqual(max.z);

            // All values should be finite
            expect(isFinite(min.x)).toBe(true);
            expect(isFinite(min.y)).toBe(true);
            expect(isFinite(min.z)).toBe(true);
            expect(isFinite(max.x)).toBe(true);
            expect(isFinite(max.y)).toBe(true);
            expect(isFinite(max.z)).toBe(true);
        });

        test('should handle out-of-bounds access gracefully', () => {
            const verticesCount = mesh.vertices.size();

            if (verticesCount > 0) {
                // Test accessing beyond bounds
                expect(() => {
                    mesh.vertices.get(verticesCount);
                }).not.toThrow();

                expect(() => {
                    mesh.vertices.get(-1);
                }).not.toThrow();
            }
        });
    });

    describe('Mesh Performance', () => {
        test('should access mesh data efficiently', () => {
            const world = zenkit.createWorld();
            const mesh = world.mesh;

            // Measure time to access various properties
            const startTime = Date.now();

            const name = mesh.name;
            const verticesCount = mesh.vertices.size();
            const featuresCount = mesh.features.size();
            const bboxMin = mesh.boundingBoxMin;
            const bboxMax = mesh.boundingBoxMax;

            const endTime = Date.now();
            const accessTime = endTime - startTime;

            // Property access should be fast (< 100ms for reasonable data)
            expect(accessTime).toBeLessThan(100);

            // Verify we got valid data
            expect(typeof name).toBe('string');
            expect(typeof verticesCount).toBe('number');
            expect(typeof featuresCount).toBe('number');
            expect(bboxMin).toBeDefined();
            expect(bboxMax).toBeDefined();
        });
    });
});

