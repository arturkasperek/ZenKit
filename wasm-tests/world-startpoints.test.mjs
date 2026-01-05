import fs from "fs";
import path from "path";

describe("World startpoints (zCVobStartpoint)", () => {
  test("exposes startpoints via getStartpoints()", async () => {
    const zenkit = await global.setupZenKit();

    const zenPath = path.join(process.cwd(), "..", "spacer-web", "public", "WORLDS", "NEWWORLD", "NEWWORLD.ZEN");
    expect(fs.existsSync(zenPath)).toBe(true);

    const buf = fs.readFileSync(zenPath);
    const world = zenkit.createWorld();
    const load = world.loadFromArray(new Uint8Array(buf), 0);
    expect(load.success).toBe(true);

    const startpoints = world.getStartpoints();
    expect(startpoints).toBeTruthy();
    expect(typeof startpoints.size).toBe("function");
    expect(startpoints.size()).toBeGreaterThan(0);

    const sp0 = startpoints.get(0);
    expect(sp0).toBeTruthy();
    // VirtualObjectType::zCVobStartpoint
    expect(sp0.type).toBe(12);
    expect(typeof (sp0.name || sp0.vobName || sp0.objectName)).toBe("string");
  });
});

