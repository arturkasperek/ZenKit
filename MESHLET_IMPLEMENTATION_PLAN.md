# Full OpenGothic Meshlet Implementation Plan

## Goal
Match OpenGothic's output exactly: 748,416 triangles with full meshlet optimization.

## What Meshlets Do

From OpenGothic logs:
- INPUT: 473,502 triangles
- OUTPUT: 748,416 triangles (+58% expansion)
- Why: Vertices are duplicated when triangles span meshlet boundaries

## Implementation Options

### Option A: Full Meshlet Implementation (Complex)
**Pros**: Perfect match with OpenGothic
**Cons**: 
- ~500+ lines of complex C++ code
- Meshlet building algorithm (greedy optimization)
- Boundary detection and splitting
- Cache-aware vertex ordering
- Only useful for Vulkan/Metal renderers

**Files needed**:
- Meshlet struct with insert/merge/flush logic
- PrimitiveHeap sorting
- Boundary detection
- Vertex cache optimization

### Option B: Simplified Vertex Expansion (Medium)
**Pros**: Easier to implement, same visual result
**Cons**: Won't match exact triangle count

Create unique vertices for each triangle (no sharing):
```cpp
for each triangle:
    for each corner (3 vertices):
        vertices.push_back({pos, normal, uv})
        indices.push_back(vertex_index++)
```

Result: 473,502 × 3 = 1,420,506 vertices (more than OpenGothic)

### Option C: Material Boundary Expansion (Recommended)
**Pros**: Close to OpenGothic's count, simpler than full meshlets
**Cons**: Not exact match

Duplicate vertices only at material boundaries:
- Within same material: share vertices
- Across materials: duplicate vertices

Expected: ~600K-700K triangles (between our 473K and OpenGothic's 748K)

## Recommended Approach

**Start with Option C** (Material Boundary Expansion):

1. Keep current implementation (473,502 triangles)
2. Add optional flag: `expandAtMaterialBoundaries`
3. When enabled:
   - Track which material each vertex belongs to
   - If vertex used by multiple materials, duplicate it
   - Results in vertex expansion similar to meshlets

4. Test visual quality with zen-viewer
5. If needed, implement full meshlets (Option A)

## Implementation Code (Option C)

```cpp
ProcessedMeshData MeshWrapper::getProcessedMeshData(bool expandBoundaries) const {
    // ... existing material deduplication ...
    // ... existing triangle sorting ...
    
    if (!expandBoundaries) {
        // Current implementation: 473K triangles
        // Shared vertices across materials
    } else {
        // Material boundary expansion
        std::unordered_map<uint64_t, std::vector<uint32_t>> vertexToMaterials;
        
        // First pass: track which materials use each vertex
        for (const auto& tri : triangles) {
            uint64_t keys[3] = {
                mkUInt64(ibo[tri.primId+0], feat[tri.primId+0]),
                mkUInt64(ibo[tri.primId+1], feat[tri.primId+1]),
                mkUInt64(ibo[tri.primId+2], feat[tri.primId+2])
            };
            
            for (auto key : keys) {
                vertexToMaterials[key].push_back(tri.matId);
            }
        }
        
        // Second pass: create separate vertices for each material
        std::unordered_map<std::pair<uint64_t, uint32_t>, uint32_t> vertexMatKey;
        
        for (const auto& tri : triangles) {
            for (int c = 0; c < 3; ++c) {
                uint64_t key = mkUInt64(...);
                auto matKey = std::make_pair(key, tri.matId);
                
                if (vertexMatKey.find(matKey) == vertexMatKey.end()) {
                    // Create new vertex for this material
                    vertexMatKey[matKey] = newVertexIndex++;
                    // Add vertex data
                }
                
                result.indices.push_back(vertexMatKey[matKey]);
            }
        }
    }
}
```

## Decision Point

**Question for user**: Which option do you prefer?

A. Full meshlet implementation (exact match, very complex)
B. Simplified expansion (easy, but over-expands)  
C. Material boundary expansion (middle ground)
D. Keep current 473K triangle implementation (simplest, correct game data)

My recommendation: **Try D first in zen-viewer**, then add C if needed for visual quality.
