/**
 * VOB collidability flags (WASM bindings)
 *
 * Ensures ZenKit exposes the key VirtualObject collision/physics flags via VobData.
 */

import fs from "fs";
import path from "path";

describe("ZenKit VOB collidability flags", () => {
  test(
    "world.getVobs() exposes cdStatic/cdDynamic/vobStatic/physicsEnabled/bbox",
    async () => {
      const zenkit = await setupZenKit();
      const world = zenkit.createWorld();

      const worldPath = path.join(process.cwd(), "TOTENINSEL.ZEN");
      expect(fs.existsSync(worldPath)).toBe(true);

      const buffer = new Uint8Array(fs.readFileSync(worldPath));
      const result = world.loadFromArray(buffer, 0);
      expect(result).toBeDefined();
      expect(result.success).toBe(true);

      const vobs = world.getVobs();
      expect(vobs).toBeDefined();
      expect(typeof vobs.size).toBe("function");
      expect(typeof vobs.get).toBe("function");

      const n = vobs.size();
      expect(n).toBeGreaterThan(0);

      const v0 = vobs.get(0);
      expect(v0).toBeDefined();
      expect(typeof v0.cdStatic).toBe("boolean");
      expect(typeof v0.cdDynamic).toBe("boolean");
      expect(typeof v0.vobStatic).toBe("boolean");
      expect(typeof v0.physicsEnabled).toBe("boolean");

      expect(v0.bbox).toBeDefined();
      expect(v0.bbox.min).toBeDefined();
      expect(v0.bbox.max).toBeDefined();
      expect(typeof v0.bbox.min.x).toBe("number");
      expect(typeof v0.bbox.max.z).toBe("number");
    },
    60000,
  );
});

