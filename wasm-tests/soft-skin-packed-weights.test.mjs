/**
 * SoftSkinMesh packed weights API
 *
 * Ensures WASM bindings expose a packed, TypedArray-based weight format.
 */

import path from "path";

describe("SoftSkinMesh getPackedWeights4()", () => {
  let zenkit;

  beforeAll(async () => {
    zenkit = await setupZenKit();
    expect(zenkit).toBeDefined();
  }, 30000);

  test("returns typed arrays with expected sizes", async () => {
    const mdhPath = path.join(process.cwd(), "public", "game-assets", "ANIMS", "_COMPILED", "TROLL.MDH");
    const mdmPath = path.join(process.cwd(), "public", "game-assets", "ANIMS", "_COMPILED", "TROLL_BLACK_BODY.MDM");

    const hierarchyBytes = new Uint8Array(await (await import("fs/promises")).readFile(mdhPath));
    const meshBytes = new Uint8Array(await (await import("fs/promises")).readFile(mdmPath));

    const hierarchyLoader = zenkit.createModelHierarchyLoader();
    const meshLoader = zenkit.createModelMeshLoader();

    expect(hierarchyLoader.loadFromArray(hierarchyBytes).success).toBe(true);
    expect(meshLoader.loadFromArray(meshBytes).success).toBe(true);

    const model = zenkit.createModel();
    model.setHierarchy(hierarchyLoader.getHierarchy());
    model.setMesh(meshLoader.getMesh());

    const soft = model.getSoftSkinMeshes();
    expect(soft).toBeDefined();
    expect(typeof soft.size).toBe("function");
    expect(soft.size()).toBeGreaterThan(0);

    const s0 = soft.get(0);
    expect(s0).toBeDefined();
    expect(typeof s0.getPackedWeights4).toBe("function");

    const packed = s0.getPackedWeights4();
    expect(packed).toBeDefined();
    expect(typeof packed.vertexCount).toBe("number");
    expect(packed.vertexCount).toBeGreaterThan(0);
    expect(packed.maxInfluences).toBe(4);

    expect(packed.boneIndices).toBeInstanceOf(Uint16Array);
    expect(packed.boneWeights).toBeInstanceOf(Float32Array);
    expect(packed.bonePositions).toBeInstanceOf(Float32Array);

    expect(packed.boneIndices.length).toBe(packed.vertexCount * 4);
    expect(packed.boneWeights.length).toBe(packed.vertexCount * 4);
    expect(packed.bonePositions.length).toBe(packed.vertexCount * 4 * 3);
  });
});

