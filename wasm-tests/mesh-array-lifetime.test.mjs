/**
 * Mesh Typed Array Lifetime Tests
 *
 * Captures the regression where typed arrays returned from WASM-backed mesh
 * can become invalid or are views into the WASM heap. This test intentionally
 * fails today if arrays are WASM-backed, so we have a UT guarding the fix.
 */

import path from 'path';
import fs from 'fs';

describe('Mesh typed array lifetime', () => {
    let zenkit;

    beforeAll(async () => {
        zenkit = await setupZenKit();
        expect(zenkit).toBeDefined();
    }, 30000);

    function loadZenIntoWorld(filename = 'TOTENINSEL.ZEN') {
        const filePath = path.join(process.cwd(), filename);
        if (!fs.existsSync(filePath)) {
            throw new Error(`Required test asset not found: ${filePath}`);
        }

        const data = fs.readFileSync(filePath);
        const bytes = new Uint8Array(data);
        const ptr = zenkit._malloc(bytes.length);
        zenkit.HEAPU8.set(bytes, ptr);

        const world = zenkit.createWorld();
        try {
            // Real WASM throws on invalid loads; should not throw for a valid ZEN
            world.load(ptr, bytes.length);
            return { world, cleanup: () => zenkit._free(ptr) };
        } catch (e) {
            zenkit._free(ptr);
            throw e;
        }
    }

    test('typed arrays should NOT be backed by WASM heap (guard for future fix)', () => {
        const { world, cleanup } = loadZenIntoWorld();
        try {
            const mesh = world.mesh;
            const vertices = mesh.getVerticesTypedArray();
            const normals = mesh.getNormalsTypedArray();
            const uvs = mesh.getUVsTypedArray();
            const indices = mesh.getIndicesTypedArray();

            // Sanity: arrays have content
            expect(vertices.length).toBeGreaterThan(0);
            expect(indices.length).toBeGreaterThan(0);

            // Deterministic check: these should not alias the WASM memory buffer
            // This will currently FAIL if getters return views into Emscripten HEAP.
            const wasmBuffer = zenkit.HEAPU8.buffer;
            expect(vertices.buffer).not.toBe(wasmBuffer);
            expect(normals.buffer).not.toBe(wasmBuffer);
            expect(uvs.buffer).not.toBe(wasmBuffer);
            expect(indices.buffer).not.toBe(wasmBuffer);
        } finally {
            cleanup();
        }
    });

    test('typed arrays remain readable after GC pressure when world reference is lost (best-effort)', () => {
        // This test is best-effort and may be skipped if Node GC is not exposed
        const canGc = typeof global.gc === 'function';
        if (!canGc) {
            console.warn('Skipping GC pressure test (run node with --expose-gc to enable)');
            return;
        }

        const { world, cleanup } = loadZenIntoWorld();
        let vertices, indices;
        try {
            const mesh = world.mesh;
            vertices = mesh.getVerticesTypedArray();
            indices = mesh.getIndicesTypedArray();
        } finally {
            // Intentionally drop world reference before GC
            // Cleanup only frees the input buffer, not internal world memory
            cleanup();
        }

        // Force GC pressure
        for (let i = 0; i < 20; i++) {
            const junk = new Array(1e5).fill(i);
            void junk;
            global.gc();
        }

        // Access previously obtained arrays; they should still be readable and consistent
        expect(vertices.length).toBeGreaterThan(0);
        expect(indices.length).toBeGreaterThan(0);
        const firstV = vertices[0];
        const firstI = indices[0];
        expect(Number.isFinite(firstV)).toBe(true);
        expect(Number.isInteger(firstI)).toBe(true);
    });
});


