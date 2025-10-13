# OpenGothic Mesh Processing Logging Guide

## What Was Added

I've added detailed logging to OpenGothic's `packMeshletsLnd` function to track exactly how the mesh is processed.

### Modified File
- `OpenGothic/game/graphics/mesh/submesh/packedmesh.cpp`

### Log Sections

The logs will show 4 key sections:

#### 1. RAW INPUT DATA
```
[PackedMesh::packMeshletsLnd] RAW INPUT DATA:
  mesh.vertices.size() = 260696
  mesh.features.size() = 711615
  mesh.materials.size() = 506
  mesh.polygons.vertex_indices.size() = 1399011
  mesh.polygons.feature_indices.size() = 1399011
  mesh.polygons.material_indices.size() = 748416
  TRIANGLE COUNT (material_indices.size) = 748416
  VERTEX INDEX COUNT / 3 = 466337
```

**Key Values:**
- `material_indices.size()` = **TRIANGLE COUNT** (this is what we need!)
- `vertex_indices.size() / 3` = vertex triangle count (different!)

#### 2. MATERIAL DEDUPLICATION
```
[PackedMesh] MATERIAL DEDUPLICATION:
  Original materials: 506
  Deduplicated materials: 338
```

Shows how many materials were merged based on visual similarity.

#### 3. TRIANGLE PROCESSING
```
[PackedMesh] TRIANGLE PROCESSING:
  Created 748416 triangles for sorting
  Triangles sorted by material
```

Confirms how many triangles go into meshlet processing.

#### 4. FINAL OUTPUT
```
[PackedMesh] FINAL OUTPUT:
  vertices.size() = 748416
  indices.size() = 2245248
  OUTPUT TRIANGLES = 748416
  subMeshes.size() = 338

[PackedMesh] SUMMARY:
  INPUT: 748416 triangles from material_indices
  OUTPUT: 748416 triangles after meshlet processing
  DIFFERENCE: 0 triangles
```

Shows final mesh data (this gets exported to JSON).

## How to Run

### Option 1: Use the script
```bash
./run_opengothic_with_logs.sh
```

### Option 2: Manual command
```bash
./OpenGothic/build/opengothic/Gothic2Notr.sh \
  -g $GOTHIC_PATH \
  -w NEWWORLD.ZEN \
  -nomenu \
  -devmode \
  -window 2>&1 | tee opengothic_mesh_processing_log.txt
```

## What to Look For

### Critical Question: Triangle Count

**ZenKit WASM reports**: 473,502 triangles
**We need to see what OpenGothic C++ gets**: ???

Look for this line:
```
mesh.polygons.material_indices.size() = ???
```

### Possible Outcomes

1. **If OpenGothic shows 748,416 triangles**:
   - The C++ ZenKit library loads MORE data than WASM
   - This is a WASM bindings bug (missing data)
   - We need to fix the WASM loading

2. **If OpenGothic shows 473,502 triangles**:
   - Both load the same raw data
   - The 748,416 in the JSON is from meshlet expansion
   - Our implementation should match (after fixing the mid.size() bug)

3. **If OpenGothic shows a different number**:
   - Something else is going on (VOB merging, loading issues, etc.)

## Extracting Just the Mesh Info

After running, extract the relevant section:

```bash
grep -B 2 -A 20 '\[PackedMesh' opengothic_mesh_processing_log.txt
```

## Next Steps

1. Run OpenGothic with the command above
2. Find the mesh processing logs (search for "[PackedMesh")
3. Note the `material_indices.size()` value
4. Compare with our ZenKit WASM value (473,502)
5. Share the log output so we can diagnose the discrepancy

## Expected Log Location

The log will be saved to:
```
/Users/artur/dev/gothic/ZenKit/opengothic_mesh_processing_log.txt
```

Look for the section starting with:
```
[PackedMesh::packMeshletsLnd] RAW INPUT DATA:
```
