// Copyright © 2024 GothicKit Contributors.
// SPDX-License-Identifier: MIT
#include "bindings_common.hh"
#include "zenkit/Stream.hh"
#include "zenkit/Texture.hh"
#include <algorithm>
#include <map>
#include <unordered_map>

namespace zenkit::wasm {

    std::unique_ptr<zenkit::Read> create_reader_from_buffer(uintptr_t data_ptr, size_t length) {
        auto bytes = reinterpret_cast<const std::byte*>(data_ptr);
        return zenkit::Read::from(bytes, length);
    }

    std::unique_ptr<zenkit::Read> create_reader_from_string(const std::string& buffer) {
        // Create a vector to avoid string encoding issues
        std::vector<std::byte> data(buffer.size());
        std::memcpy(data.data(), buffer.data(), buffer.size());
        return zenkit::Read::from(std::move(data));
    }

    // New: Create reader from JavaScript Uint8Array (handles memory automatically)
    std::unique_ptr<zenkit::Read> create_reader_from_js_array(const emscripten::val& uint8_array) {
        // Get the length and data pointer from JavaScript Uint8Array
        auto length = uint8_array["length"].as<size_t>();
        
        // Copy data from JavaScript to C++ to avoid memory management issues
        std::vector<std::byte> data(length);
        
        // Copy data from JavaScript array to our C++ vector
        emscripten::val memory = emscripten::val::module_property("HEAPU8");
        emscripten::val memoryBuffer = uint8_array["buffer"];
        emscripten::val byteOffset = uint8_array["byteOffset"];
        
        // Use JavaScript's subarray to get a view of the data
        emscripten::val dataView = uint8_array.call<emscripten::val>("subarray", 0, length);
        
        // Copy byte by byte (safer for WASM)
        for (size_t i = 0; i < length; ++i) {
            auto byte_val = uint8_array[i].as<uint8_t>();
            data[i] = static_cast<std::byte>(byte_val);
        }
        
        return zenkit::Read::from(std::move(data));
    }

    std::unique_ptr<ReadArchiveWrapper> create_read_archive(uintptr_t data_ptr, size_t length) {
        auto reader = create_reader_from_buffer(data_ptr, length);
        auto archive = zenkit::ReadArchive::from(reader.get());
        return std::make_unique<ReadArchiveWrapper>(std::move(archive));
    }

    // New: Create archive from JavaScript Uint8Array
    std::unique_ptr<ReadArchiveWrapper> create_read_archive_from_js_array(const emscripten::val& uint8_array) {
        auto reader = create_reader_from_js_array(uint8_array);
        auto archive = zenkit::ReadArchive::from(reader.get());
        return std::make_unique<ReadArchiveWrapper>(std::move(archive));
    }

    Result<bool> TextureWrapper::loadFromArray(const emscripten::val& uint8_array) {
        try {
            auto reader = create_reader_from_js_array(uint8_array);
            tex_.load(reader.get());
            return Result<bool>(true);
        } catch (const std::exception& e) {
            return Result<bool>(e.what());
        }
    }

    emscripten::val TextureWrapper::asRgba8(uint32_t mip_level) const {
        try {
            auto data = tex_.as_rgba8(mip_level);
            if(data.empty())
                return emscripten::val::null();
            emscripten::val Uint8Array = emscripten::val::global("Uint8Array");
            emscripten::val js_array = Uint8Array.new_(data.size());
            js_array.call<void>("set", emscripten::val(emscripten::typed_memory_view(data.size(), data.data())));
            return js_array;
        } catch(...) {
            return emscripten::val::null();
        }
    }

    // Helper to create composite key from vertex and feature indices
    static inline uint64_t mkUInt64(uint32_t a, uint32_t b) {
        return (uint64_t(a) << 32) | uint64_t(b);
    }

    // Material comparison function matching OpenGothic's isVisuallySame
    bool MeshWrapper::isVisuallySame(const zenkit::Material& a, const zenkit::Material& b) {
        return
            a.group                        == b.group &&
            a.color.r                      == b.color.r &&
            a.color.g                      == b.color.g &&
            a.color.b                      == b.color.b &&
            a.color.a                      == b.color.a &&
            a.smooth_angle                 == b.smooth_angle &&
            a.texture                      == b.texture &&
            a.texture_scale.x              == b.texture_scale.x &&
            a.texture_scale.y              == b.texture_scale.y &&
            a.texture_anim_fps             == b.texture_anim_fps &&
            a.texture_anim_map_mode        == b.texture_anim_map_mode &&
            a.texture_anim_map_dir.x       == b.texture_anim_map_dir.x &&
            a.texture_anim_map_dir.y       == b.texture_anim_map_dir.y &&
            a.detail_object                == b.detail_object &&
            a.detail_object_scale          == b.detail_object_scale &&
            a.force_occluder               == b.force_occluder &&
            a.environment_mapping          == b.environment_mapping &&
            a.environment_mapping_strength == b.environment_mapping_strength &&
            a.wave_mode                    == b.wave_mode &&
            a.wave_speed                   == b.wave_speed &&
            a.wave_max_amplitude           == b.wave_max_amplitude &&
            a.wave_grid_size               == b.wave_grid_size &&
            a.ignore_sun                   == b.ignore_sun &&
            a.default_mapping.x            == b.default_mapping.x &&
            a.default_mapping.y            == b.default_mapping.y;
    }

    // ProcessedMeshData implementation matching OpenGothic's packMeshletsLnd
    ProcessedMeshData MeshWrapper::getProcessedMeshData() const {
        ProcessedMeshData result;
        
        const auto& ibo  = mesh_.polygons.vertex_indices;
        const auto& feat = mesh_.polygons.feature_indices;
        const auto& mid  = mesh_.polygons.material_indices;
        
        if (ibo.empty() || mesh_.materials.empty()) {
            return result; // Empty mesh
        }
        
        // Safety check: ensure indices arrays are compatible
        if (ibo.size() != feat.size()) {
            return result; // Incompatible data
        }
        
        // Step 1: Build material deduplication map
        std::vector<uint32_t> mat(mesh_.materials.size());
        for (size_t i = 0; i < mesh_.materials.size(); ++i) {
            mat[i] = uint32_t(i);
        }
        
        // Deduplicate materials by finding visually identical ones
        for (size_t i = 0; i < mesh_.materials.size(); ++i) {
            for (size_t r = i + 1; r < mesh_.materials.size(); ++r) {
                if (mat[i] == mat[r])
                    continue;
                const auto& a = mesh_.materials[i];
                const auto& b = mesh_.materials[r];
                if (isVisuallySame(a, b)) {
                    mat[r] = mat[i]; // Point r to i's deduplicated index
                }
            }
        }
        
        // Build deduplicated materials list
        std::map<uint32_t, uint32_t> matIdxRemap; // old -> new index in deduplicated list
        for (size_t i = 0; i < mesh_.materials.size(); ++i) {
            uint32_t dedupIdx = mat[i];
            if (matIdxRemap.find(dedupIdx) == matIdxRemap.end()) {
                uint32_t newIdx = static_cast<uint32_t>(result.materials.size());
                matIdxRemap[dedupIdx] = newIdx;
                result.materials.emplace_back(mesh_.materials[dedupIdx]);
            }
        }
        
        // Step 2: Create triangle list with deduplicated material IDs
        struct Triangle {
            uint32_t primId; // triangle index * 3
            uint32_t matId;  // deduplicated material ID
        };
        
        size_t triCount = mid.size(); // Use material_indices size, which is the triangle count
        std::vector<Triangle> triangles;
        triangles.reserve(triCount);
        
        for (size_t i = 0; i < triCount; ++i) {
            uint32_t originalMatIdx = mid[i];
            if (originalMatIdx < mat.size()) {
                uint32_t dedupMatIdx = mat[originalMatIdx];
                Triangle tri;
                tri.primId = uint32_t(i * 3);
                tri.matId = matIdxRemap[dedupMatIdx];
                triangles.push_back(tri);
            }
        }
        
        // Step 3: Sort triangles by material (matching OpenGothic's sorting)
        std::sort(triangles.begin(), triangles.end(), [](const Triangle& a, const Triangle& b) {
            return a.matId < b.matId;
        });
        
        // Step 4: Process vertices with composite (vertex, feature) keys
        std::unordered_map<uint64_t, uint32_t> vertexMap; // composite key -> new vertex index
        vertexMap.reserve(triangles.size()); // Reserve space to avoid rehashing
        
        const size_t featureCount = mesh_.features.size();
        const size_t vertexCount = mesh_.vertices.size();
        
        result.indices.reserve(triangles.size() * 3);
        result.materialIds.reserve(triangles.size());
        result.vertices.reserve(triangles.size() * 3 * 8); // Estimate for unique vertices
        
        for (const auto& tri : triangles) {
            result.materialIds.push_back(tri.matId);
            
            for (int c = 0; c < 3; ++c) {
                // Bounds check before accessing
                if (tri.primId + c >= ibo.size()) {
                    continue; // Skip invalid triangle
                }
                
                uint32_t vi = ibo[tri.primId + c];
                uint32_t fi = feat[tri.primId + c];
                
                // Apply the critical bit-shift fix from Gothic engine
                // "if (featIndex>=numFeatList) featIndex = featIndex >> 16;"
                if (fi >= featureCount) {
                    fi = fi >> 16;
                }
                
                // Additional bounds check after fix
                if (vi >= vertexCount || fi >= featureCount) {
                    // Use fallback for invalid indices
                    vi = 0;
                    fi = 0;
                }
                
                // Create composite key
                uint64_t key = mkUInt64(vi, fi);
                
                // Check if this vertex combination already exists
                auto it = vertexMap.find(key);
                if (it != vertexMap.end()) {
                    // Reuse existing vertex
                    result.indices.push_back(it->second);
                } else {
                    // Create new vertex
                    uint32_t newIdx = static_cast<uint32_t>(result.vertices.size() / 8);
                    vertexMap[key] = newIdx;
                    result.indices.push_back(newIdx);
                    
                    // Add vertex data: [x,y,z, nx,ny,nz, u,v]
                    if (vi < mesh_.vertices.size()) {
                        const auto& v = mesh_.vertices[vi];
                        result.vertices.push_back(v.x);
                        result.vertices.push_back(v.y);
                        result.vertices.push_back(v.z);
                    } else {
                        result.vertices.push_back(0.0f);
                        result.vertices.push_back(0.0f);
                        result.vertices.push_back(0.0f);
                    }
                    
                    if (fi < mesh_.features.size()) {
                        const auto& f = mesh_.features[fi];
                        result.vertices.push_back(f.normal.x);
                        result.vertices.push_back(f.normal.y);
                        result.vertices.push_back(f.normal.z);
                        result.vertices.push_back(f.texture.x);
                        result.vertices.push_back(f.texture.y);
                    } else {
                        result.vertices.push_back(0.0f);
                        result.vertices.push_back(0.0f);
                        result.vertices.push_back(1.0f);
                        result.vertices.push_back(0.0f);
                        result.vertices.push_back(0.0f);
                    }
                }
            }
        }
        
        return result;
    }

} // namespace zenkit::wasm
