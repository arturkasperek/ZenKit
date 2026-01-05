import fs from "fs";
import path from "path";

describe("DaedalusVm array member indexing", () => {
  test("reads int array elements via [index] and [CONST]", async () => {
    const zenkit = await global.setupZenKit();

    const datPath = path.join(process.cwd(), "..", "spacer-web", "public", "SCRIPTS", "_COMPILED", "GOTHIC.DAT");
    expect(fs.existsSync(datPath)).toBe(true);

    const scriptBuf = fs.readFileSync(datPath);
    const script = zenkit.createDaedalusScript();
    const load = script.loadFromArray(new Uint8Array(scriptBuf));
    expect(load.success).toBe(true);

    const vm = zenkit.createDaedalusVm(script);
    vm.setDefaultExternalHandler(() => {});

    const instName = "MIL_309_STADTWACHE";

    let instIdx = -1;
    for (let i = 0; i < vm.symbolCount; i++) {
      const r = vm.getSymbolNameByIndex(i);
      if (r.success && r.data === instName) {
        instIdx = i;
        break;
      }
    }
    expect(instIdx).toBeGreaterThan(0);

    vm.setGlobalSelf(instName);
    vm.initInstanceByIndex(instIdx);

    const hp0 = vm.getSymbolInt("C_NPC.attribute", instName);
    expect(hp0).toBeGreaterThan(0);

    expect(vm.getSymbolInt("C_NPC.attribute[0]", instName)).toBe(hp0);
    expect(vm.getSymbolInt("C_NPC.attribute[ATR_HITPOINTS]", instName)).toBe(hp0);

    const hpMax = vm.getSymbolInt("C_NPC.attribute[1]", instName);
    expect(hpMax).toBeGreaterThan(0);
    expect(vm.getSymbolInt("C_NPC.attribute[ATR_HITPOINTS_MAX]", instName)).toBe(hpMax);
  });
});

