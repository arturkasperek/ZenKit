# ZenKit WASM Processing - Testing Guide

## What We've Built

✅ ZenKit WASM with `getProcessedMeshData()` method
✅ Material deduplication (1,400 → 352 materials)
✅ Composite vertex processing (position + normal + UV)
✅ Feature index bit-shift fix
✅ Triangle sorting by material
✅ Updated zen-viewer.html to use processed data
✅ Comparison script with OpenGothic

## Quick Start - Test the Viewer

### 1. Start Local Server

```bash
# If you have Python:
python3 -m http.server 8080

# Or if you have Node.js http-server:
npx http-server -p 8080
```

### 2. Open the Viewer

Navigate to: `http://localhost:8080/examples/zen-viewer.html`

**What to expect:**
- NEWWORLD.ZEN loads automatically (it's ~72MB, takes a few seconds)
- You should see the Gothic world rendered in 3D
- Console will show: "Processed mesh: 707658 vertices, 473502 triangles, 352 materials"

### 3. Check Browser Console

Open Developer Tools (F12) and look for:

```
🔧 Using new getProcessedMeshData() with OpenGothic pipeline...
Processed mesh: 707658 vertices, 473502 triangles, 352 materials (deduplicated)
✅ 3D mesh created successfully with processed data!
   - 352 deduplicated materials
   - 707658 composite vertices
   - 473502 triangles (sorted by material)
```

## Detailed Testing

### Test 1: Compare with OpenGothic Data

```bash
cd /Users/artur/dev/gothic/ZenKit
node --max-old-space-size=8192 examples/compare_zenkit_opengothic.mjs
```

**What to look for:**
```
Total triangles:
  ZenKit:     473,502
  OpenGothic: 748,416
  Ratio: 0.633

Total materials:
  ZenKit:     352
  OpenGothic: 338
```

**This is EXPECTED!**
- 473K = raw game data (correct)
- 748K = OpenGothic's meshlet optimization (not needed for Three.js)

### Test 2: Test Different ZEN Files

Edit `examples/zen-viewer.html` line 199:

```javascript
// Try different files:
const response = await fetch('/TOTENINSEL.ZEN');  // Small test file
// or
const response = await fetch('/public/game-assets/WORLDS/NEWWORLD/DRAGONISLAND.ZEN');
```

Then reload the browser.

### Test 3: Verify Processing Stats

```bash
# Test DRAGONISLAND.ZEN
node test_dragonisland_main.mjs

# Test TOTENINSEL.ZEN
node test_toteninsel.mjs
```

**Expected output:**
```
Processed mesh data:
  Processed vertices: 116,280
  Processed indices: 279,006
  Triangles: 93,002
  Material IDs: 93,002
  Deduplicated materials: 81
```

## What to Check Visually

### ✅ Good Signs:
- World geometry loads and displays
- Textures load progressively
- No black holes or missing geometry
- Materials group correctly (similar textures together)
- Can orbit camera smoothly

### ❌ Problems to Look For:
- Black screen (check console for errors)
- Missing textures (some textures may not load - this is OK)
- Geometry holes (indicates vertex processing issue)
- Flickering (z-fighting - we have fixes for this)
- Wrong colors/materials

## Debug Mode

If something doesn't work, check:

### 1. Browser Console Errors

Look for:
- "Failed to create 3D mesh" - WASM issue
- "Error loading texture" - texture path issue (expected for some textures)
- Any JavaScript errors

### 2. Network Tab

Check if files are loading:
- `zenkit.wasm` - should load (~450KB)
- `NEWWORLD.ZEN` - should load (~72MB)
- `.TEX` files - textures (many will 404 - this is OK)

### 3. WASM Console Logs

In the console, you should see:
```
[C++] getProcessedMeshData() called
[C++] Mesh data: vertices=260696, features=711615, indices=1420506
[C++] Starting material deduplication...
[C++] Deduplicated materials: 352
[C++] Created 473502 triangles
[C++] Triangles sorted by material...
[C++] Processing complete!
```

If you DON'T see these, WASM didn't load properly.

## Performance Expectations

| File | Size | Load Time | Triangles | FPS |
|------|------|-----------|-----------|-----|
| TOTENINSEL.ZEN | 0.8 MB | 1-2s | 7,554 | 60 |
| DRAGONISLAND.ZEN | 16 MB | 3-5s | 93,002 | 45-60 |
| NEWWORLD.ZEN | 72 MB | 8-15s | 473,502 | 30-45 |

## Common Issues & Solutions

### Issue: Black screen after loading

**Solution**: Check console for errors. Most common:
```javascript
// In zen-viewer.html, add this after line 252:
console.log("Mesh loaded, calling getProcessedMeshData...");
const processed = zenMesh.getProcessedMeshData();
console.log("Processed data:", processed);
```

### Issue: "getProcessedMeshData is not a function"

**Solution**: WASM wasn't rebuilt. Run:
```bash
npm run build:wasm
```

Then hard-reload browser (Cmd+Shift+R on Mac, Ctrl+Shift+R on Windows).

### Issue: Textures don't load

**Expected!** Some textures will be missing. You should see:
- Some objects with colored materials (fallback)
- Some objects with textures (loaded successfully)

This is normal - not all TEX files are in your game assets folder.

### Issue: Memory error when loading NEWWORLD.ZEN

**Solution**: Use auto-detect mode (already set). If it still fails, try DRAGONISLAND.ZEN instead.

## File Locations

```
/Users/artur/dev/gothic/ZenKit/
├── examples/zen-viewer.html          ← Main viewer (open in browser)
├── examples/compare_zenkit_opengothic.mjs  ← Comparison script
├── build-wasm/wasm/
│   ├── zenkit.wasm                   ← WASM binary
│   └── zenkit.mjs                    ← WASM module
├── test_toteninsel.mjs               ← Test script for small file
├── test_dragonisland_main.mjs        ← Test script for medium file
└── public/game-assets/               ← Your Gothic data
    ├── WORLDS/NEWWORLD/NEWWORLD.ZEN
    ├── WORLDS/NEWWORLD/DRAGONISLAND.ZEN
    └── TEXTURES/_COMPILED/*.TEX
```

## Next Steps After Testing

1. **If it works**: Great! You have a working Gothic world viewer
2. **If textures are missing**: Add more `.TEX` files to `/public/game-assets/TEXTURES/_COMPILED/`
3. **If geometry looks wrong**: Share screenshots and console logs
4. **If performance is bad**: Try smaller worlds (DRAGONISLAND, TOTENINSEL)

## Share Results

When reporting results, include:

```bash
# Browser info
# What file you loaded (NEWWORLD.ZEN, etc.)
# Console logs (F12 → Console → copy all)
# Screenshot of the result
# Any errors from browser console
```

This will help debug any issues!
