// Copyright © 2024 GothicKit Contributors.
// SPDX-License-Identifier: MIT
#include "bindings_common.hh"
#include "zenkit/Stream.hh"
#include "zenkit/Texture.hh"
#include "zenkit/World.hh"
#include "zenkit/Model.hh"
#include "zenkit/ModelHierarchy.hh"
#include "zenkit/ModelMesh.hh"
#include "zenkit/MultiResolutionMesh.hh"
#include "zenkit/SoftSkinMesh.hh"
#include "zenkit/vobs/VirtualObject.hh"
#include "zenkit/DaedalusScript.hh"
#include "zenkit/DaedalusVm.hh"
#include "zenkit/CutsceneLibrary.hh"
#include "zenkit/ModelScript.hh"
#include "zenkit/Archive.hh"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <unordered_map>
#include <variant>

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

    // VobData constructor implementation
    VobData::VobData(const zenkit::VirtualObject& vob)
        : id(vob.id)
        , vob_name(vob.vob_name)
        , type(static_cast<uint32_t>(vob.type))
        , position(vob.position)
        , rotation(vob.rotation)
        , visual(*vob.visual)
        , show_visual(vob.show_visual)
        , cd_dynamic(vob.cd_dynamic) {
        
        // Recursively convert children
        children.reserve(vob.children.size());
        for (const auto& child : vob.children) {
            children.emplace_back(*child);
        }
    }

    // StandaloneMeshWrapper implementation
    Result<bool> StandaloneMeshWrapper::loadFromArray(const emscripten::val& uint8_array) {
        try {
            auto reader = create_reader_from_js_array(uint8_array);
            mesh_.load(reader.get(), false); // false = don't force wide indices
            is_mrm_ = false;
            return Result<bool>(true);
        } catch (const std::exception& e) {
            return Result<bool>(e.what());
        }
    }

    Result<bool> StandaloneMeshWrapper::loadMRMFromArray(const emscripten::val& uint8_array) {
        try {
            auto reader = create_reader_from_js_array(uint8_array);
            mrm_.load(reader.get());
            is_mrm_ = true;
            
            // Convert MRM to Mesh for rendering
            // MRM has: positions, normals, sub_meshes (with triangles, wedges), materials
            mesh_.vertices = mrm_.positions;
            mesh_.materials = mrm_.materials;
            mesh_.bbox = mrm_.bbox;
            mesh_.obb = mrm_.obbox;
            
            // Build features from wedges (MRM uses wedges for normals+UVs per vertex)
            // We need to create a feature for each unique wedge
            mesh_.features.clear();
            mesh_.polygons.vertex_indices.clear();
            mesh_.polygons.material_indices.clear();
            mesh_.polygons.feature_indices.clear();
            
            uint32_t material_idx = 0;
            for (const auto& submesh : mrm_.sub_meshes) {
                for (const auto& tri : submesh.triangles) {
                    // Add triangle indices - each wedge becomes a feature
                    for (int i = 0; i < 3; ++i) {
                        auto wedge_idx = tri.wedges[i];
                        if (wedge_idx < submesh.wedges.size()) {
                            const auto& wedge = submesh.wedges[wedge_idx];
                            
                            // Add vertex index
                            mesh_.polygons.vertex_indices.push_back(wedge.index);
                            
                            // Create feature from wedge
                            uint32_t feature_idx = mesh_.features.size();
                            zenkit::VertexFeature feat;
                            feat.normal = wedge.normal;
                            feat.texture = wedge.texture;
                            feat.light = 0xFFFFFFFF;
                            mesh_.features.push_back(feat);
                            
                            mesh_.polygons.feature_indices.push_back(feature_idx);
                        }
                    }
                    mesh_.polygons.material_indices.push_back(material_idx);
                }
                material_idx++;
            }
            
            // Copy to polygon_*_indices for compatibility
            mesh_.polygon_vertex_indices = mesh_.polygons.vertex_indices;
            mesh_.polygon_feature_indices = mesh_.polygons.feature_indices;
            
            return Result<bool>(true);
        } catch (const std::exception& e) {
            return Result<bool>(e.what());
        }
    }

    // ModelWrapper method implementations
    Result<bool> ModelWrapper::load(uintptr_t data_ptr, size_t length) {
        try {
            auto reader = create_reader_from_buffer(data_ptr, length);
            model_.load(reader.get());
            last_error_.clear();
            return Result<bool>(true);
        } catch (const std::exception& e) {
            last_error_ = e.what();
            return Result<bool>(e.what());
        }
    }

    Result<bool> ModelWrapper::loadFromArray(const emscripten::val& uint8_array) {
        try {
            auto reader = create_reader_from_js_array(uint8_array);
            model_.load(reader.get());
            last_error_.clear();
            return Result<bool>(true);
        } catch (const std::exception& e) {
            last_error_ = e.what();
            return Result<bool>(e.what());
        }
    }

    bool ModelWrapper::isLoaded() const {
        return last_error_.empty() && (!model_.hierarchy.nodes.empty() || !model_.mesh.meshes.empty());
    }

    std::vector<std::string> ModelWrapper::getAttachmentNames() const {
        std::vector<std::string> names;
        names.reserve(model_.mesh.attachments.size());
        for (const auto& pair : model_.mesh.attachments) {
            names.push_back(pair.first);
        }
        return names;
    }

    const zenkit::MultiResolutionMesh* ModelWrapper::getAttachment(const std::string& name) const {
        auto it = model_.mesh.attachments.find(name);
        return it != model_.mesh.attachments.end() ? &it->second : nullptr;
    }

    ProcessedMeshData ModelWrapper::convertAttachmentToProcessedMesh(const zenkit::MultiResolutionMesh* attachment) const {
        if (!attachment) {
            return ProcessedMeshData{}; // Return empty data
        }
        ProcessedMeshData result;

        // Convert positions and normals
        size_t vertex_count = (*attachment).positions.size();
        result.vertices.reserve(vertex_count * 8); // x,y,z,nx,ny,nz,u,v

        // For MultiResolutionMesh, we need to build vertex data from submeshes
        // Each submesh has wedges with position/normal/uv data
        uint32_t current_material_index = 0;

        for (const auto& submesh : (*attachment).sub_meshes) {
            // Add the submesh material to our materials list
            MaterialData mat_data;
            mat_data.name = submesh.mat.name;
            mat_data.group = static_cast<uint32_t>(submesh.mat.group);
            mat_data.texture = submesh.mat.texture;
            result.materials.push_back(mat_data);

            for (const auto& wedge : submesh.wedges) {
                // Position
                const auto& pos = (*attachment).positions[wedge.index];
                result.vertices.push_back(pos.x);
                result.vertices.push_back(pos.y);
                result.vertices.push_back(pos.z);

                // Normal
                const auto& normal = (*attachment).normals[wedge.index];
                result.vertices.push_back(normal.x);
                result.vertices.push_back(normal.y);
                result.vertices.push_back(normal.z);

                // UV coordinates
                result.vertices.push_back(wedge.texture.x);
                result.vertices.push_back(wedge.texture.y);
            }

            // Add triangles
            for (const auto& triangle : submesh.triangles) {
                // Convert wedge indices to vertex indices in our result array
                size_t base_index = result.vertices.size() / 8 - submesh.wedges.size();
                result.indices.push_back(base_index + triangle.wedges[0]);
                result.indices.push_back(base_index + triangle.wedges[1]);
                result.indices.push_back(base_index + triangle.wedges[2]);

                // All triangles in this submesh use the same material index
                result.materialIds.push_back(current_material_index);
            }

            current_material_index++;
        }

        return result;
    }

    ProcessedMeshData ModelWrapper::convertSoftSkinMeshToProcessedMesh(const zenkit::SoftSkinMesh* softSkinMesh) const {
        if (!softSkinMesh) {
            return ProcessedMeshData{}; // Return empty data
        }
        // SoftSkinMesh contains a MultiResolutionMesh, so we can reuse the same conversion logic
        return convertAttachmentToProcessedMesh(&softSkinMesh->mesh);
    }

    // MorphMeshWrapper method implementations
    Result<bool> MorphMeshWrapper::load(uintptr_t data_ptr, size_t length) {
        try {
            auto reader = create_reader_from_buffer(data_ptr, length);
            morph_mesh_.load(reader.get());

            last_error_.clear();
            return Result<bool>(true);
        } catch (const std::exception& e) {
            last_error_ = e.what();
            return Result<bool>(e.what());
        }
    }

    Result<bool> MorphMeshWrapper::loadFromArray(const emscripten::val& uint8_array) {
        try {
            auto reader = create_reader_from_js_array(uint8_array);
            morph_mesh_.load(reader.get());

            last_error_.clear();
            return Result<bool>(true);
        } catch (const std::exception& e) {
            last_error_ = e.what();
            return Result<bool>(e.what());
        }
    }

    bool MorphMeshWrapper::isLoaded() const {
        return last_error_.empty() && !morph_mesh_.mesh.sub_meshes.empty();
    }

    ProcessedMeshData MorphMeshWrapper::convertToProcessedMesh() const {
        ProcessedMeshData result;

        // For MultiResolutionMesh, we need to build vertex data from submeshes
        // Each submesh has wedges with position/normal/uv data
        uint32_t current_vertex_offset = 0;
        uint32_t current_material_index = 0;

        for (const auto& submesh : morph_mesh_.mesh.sub_meshes) {
            // Add the submesh material to our materials list
            MaterialData mat_data;
            mat_data.name = submesh.mat.name;
            mat_data.group = static_cast<uint32_t>(submesh.mat.group);
            mat_data.texture = submesh.mat.texture;
            result.materials.push_back(mat_data);

            // Process wedges for this submesh
            for (const auto& wedge : submesh.wedges) {
                // Position
                result.vertices.push_back(morph_mesh_.mesh.positions[wedge.index].x);
                result.vertices.push_back(morph_mesh_.mesh.positions[wedge.index].y);
                result.vertices.push_back(morph_mesh_.mesh.positions[wedge.index].z);

                // Normal
                result.vertices.push_back(wedge.normal.x);
                result.vertices.push_back(wedge.normal.y);
                result.vertices.push_back(wedge.normal.z);

                // UV coordinates
                result.vertices.push_back(wedge.texture.x);
                result.vertices.push_back(wedge.texture.y);
            }

            // Process triangles
            for (const auto& triangle : submesh.triangles) {
                result.indices.push_back(triangle.wedges[0] + current_vertex_offset);
                result.indices.push_back(triangle.wedges[1] + current_vertex_offset);
                result.indices.push_back(triangle.wedges[2] + current_vertex_offset);

                // Each triangle in this submesh uses the same material index
                result.materialIds.push_back(current_material_index);
            }

            current_vertex_offset += submesh.wedges.size();
            current_material_index++;
        }

        return result;
    }

    std::vector<std::string> MorphMeshWrapper::getAnimationNames() const {
        std::vector<std::string> names;
        names.reserve(morph_mesh_.animations.size());
        for (const auto& anim : morph_mesh_.animations) {
            names.push_back(anim.name);
        }
        return names;
    }

    // ModelHierarchyWrapper method implementations
    Result<bool> ModelHierarchyWrapper::loadFromArray(const emscripten::val& uint8_array) {
        try {
            auto reader = create_reader_from_js_array(uint8_array);
            hierarchy_.load(reader.get());
            last_error_.clear();
            return Result<bool>(true);
        } catch (const std::exception& e) {
            last_error_ = e.what();
            return Result<bool>(e.what());
        }
    }

    // ModelMeshWrapper method implementations
    Result<bool> ModelMeshWrapper::loadFromArray(const emscripten::val& uint8_array) {
        try {
            auto reader = create_reader_from_js_array(uint8_array);
            mesh_.load(reader.get());
            last_error_.clear();
            return Result<bool>(true);
        } catch (const std::exception& e) {
            last_error_ = e.what();
            return Result<bool>(e.what());
        }
    }

    // DaedalusScriptWrapper implementation
    Result<bool> DaedalusScriptWrapper::loadFromArray(const emscripten::val& uint8_array) {
        try {
            auto reader = create_reader_from_js_array(uint8_array);
            script_.load(reader.get());
            
            // Register common classes as opaque types so we can create DaedalusOpaqueInstance objects
            // This sets up member offsets without needing registered instance types like IItem
            // We register the most common classes that are likely to be accessed
            const char* common_classes[] = {
                "C_ITEM", "C_NPC", "C_INFO", "C_MISSION", "C_FOCUS", 
                "C_ITEMREACT", "C_SPELL", "C_MENU", "C_MENU_ITEM"
            };
            
            for (const char* class_name : common_classes) {
                try {
                    auto* sym = script_.find_symbol_by_name(class_name);
                    if (sym != nullptr && sym->type() == zenkit::DaedalusDataType::CLASS) {
                        script_.register_as_opaque(sym);
                    }
                } catch (...) {
                    // Class might not exist in this script, continue
                }
            }
            
            return Result<bool>(true);
        } catch (const std::exception& e) {
            last_error_ = e.what();
            return Result<bool>(e.what());
        }
    }

    // DaedalusVmWrapper implementation
    DaedalusVmWrapper::DaedalusVmWrapper(DaedalusScriptWrapper* script)
        : vm_(std::move(script->getScript())) {
        // Note: Script classes are already registered in DaedalusScriptWrapper::loadFromArray
        // before the script is moved into the VM
        
        // Register a default external handler to prevent exceptions from unregistered externals
        // This is essential for script initialization functions that might call externals
        vm_.register_default_external([](zenkit::DaedalusSymbol const& sym) {
            // Print a friendly message about unimplemented external functions
            std::cerr << "⚠️  VM: External function '" << sym.name() << "' is not implemented (called but not registered)" << std::endl;
            // The VM will automatically handle stack cleanup and return default values
        });
    }
    
    void DaedalusVmWrapper::registerDefaultExternal() {
        // This is a no-op now since we register it in the constructor
        // But kept for API compatibility
    }

    Result<bool> DaedalusVmWrapper::setDefaultExternalHandler(const emscripten::val& callback) {
        try {
            std::string callbackType = callback.typeOf().as<std::string>();
            if (callbackType != "function") {
                return Result<bool>("Callback must be a function");
            }
            
            vm_.register_default_external([callback](zenkit::DaedalusSymbol const& sym) {
                try {
                    // Call JavaScript callback with function name
                    callback(emscripten::val(sym.name()));
                } catch (const std::exception& e) {
                    std::cerr << "Error in default external handler callback: " << e.what() << std::endl;
                } catch (...) {
                    std::cerr << "Unknown error in default external handler callback" << std::endl;
                }
            });
            
            return Result<bool>(true);
        } catch (const std::exception& e) {
            return Result<bool>(e.what());
        } catch (...) {
            return Result<bool>("Unknown error setting default external handler");
        }
    }

    bool DaedalusVmWrapper::hasSymbol(const std::string& name) const {
        try {
            auto* sym = vm_.find_symbol_by_name(name);
            return sym != nullptr;
        } catch (...) {
            return false;
        }
    }

    size_t DaedalusVmWrapper::getSymbolCount() const {
        return vm_.symbols().size();
    }

    // Helper: Get or create an instance for accessing member values
    std::shared_ptr<zenkit::DaedalusInstance> DaedalusVmWrapper::getInstance(zenkit::DaedalusSymbol* instanceSym) {
        try {
            return instanceSym->get_instance();
        } catch (...) {
            // Instance not initialized - return nullptr
            // ZenKit user should ensure instances are initialized before accessing properties
            return nullptr;
        }
    }

    // Helper: Find member symbol with case-insensitive fallback
    zenkit::DaedalusSymbol* DaedalusVmWrapper::findMemberSymbol(zenkit::DaedalusSymbol* instanceSym, const std::string& symbolName) {
        zenkit::DaedalusSymbol* memberSym = nullptr;
        
        // Approach 1: Try qualified name like "C_ITEM.VISUAL" (uppercase, as registered)
        if (instanceSym->parent() != static_cast<uint32_t>(-1)) {
            try {
                auto* parentSym = vm_.find_symbol_by_index(instanceSym->parent());
                if (parentSym && parentSym->type() == zenkit::DaedalusDataType::CLASS) {
                    // Try uppercase first (as registered in daedalus.cc)
                    std::string qualifiedNameUpper = parentSym->name() + "." + symbolName;
                    std::transform(qualifiedNameUpper.begin(), qualifiedNameUpper.end(), 
                                  qualifiedNameUpper.begin(), ::toupper);
                    memberSym = vm_.find_symbol_by_name(qualifiedNameUpper);
                    
                    // If not found, try original case
                    if (!memberSym) {
                        std::string qualifiedName = parentSym->name() + "." + symbolName;
                        memberSym = vm_.find_symbol_by_name(qualifiedName);
                    }
                }
            } catch (...) {
                // Parent lookup failed, continue to next approach
            }
        }
        
        // Approach 2: Try uppercase member name
        if (!memberSym) {
            try {
                std::string upperName = symbolName;
                std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
                memberSym = vm_.find_symbol_by_name(upperName);
            } catch (...) {
                // Continue to next approach
            }
        }
        
        // Approach 3: Try original case member name
        if (!memberSym) {
            try {
                memberSym = vm_.find_symbol_by_name(symbolName);
            } catch (...) {
                return nullptr;
            }
        }
        
        return memberSym;
    }

    // Helper to convert Windows-1250 encoded string to UTF-8
    std::string convertWindows1250ToUtf8(const std::string& cp1250_str) {
        // Use JavaScript TextDecoder for conversion via Emscripten
        // Works in both browser and Node.js environments
        try {
            // Create a Uint8Array from the Windows-1250 bytes
            emscripten::val uint8Array = emscripten::val::global("Uint8Array").new_(cp1250_str.length());
            for (size_t i = 0; i < cp1250_str.length(); ++i) {
                uint8Array.set(i, static_cast<unsigned char>(cp1250_str[i]));
            }
            
            // Try to get TextDecoder (works in both browser and Node.js)
            emscripten::val TextDecoder = emscripten::val::global("TextDecoder");
            if (TextDecoder.isUndefined()) {
                // Fallback: try util.TextDecoder for Node.js
                emscripten::val util = emscripten::val::global("require").call<emscripten::val>("call", emscripten::val::null(), emscripten::val("util"));
                if (!util.isUndefined()) {
                    TextDecoder = util["TextDecoder"];
                }
            }
            
            if (!TextDecoder.isUndefined()) {
                emscripten::val decoder = TextDecoder.new_(emscripten::val("windows-1250"));
                emscripten::val utf8_str = decoder.call<emscripten::val>("decode", uint8Array);
                return utf8_str.as<std::string>();
            }
        } catch (...) {
            // Fallback: return original string if conversion fails
        }
        return cp1250_str;
    }

    std::string DaedalusVmWrapper::getSymbolString(const std::string& symbolName, const std::string& instanceName) {
        try {
            if (!instanceName.empty()) {
                // Looking up an instance member
                auto* instanceSym = vm_.find_symbol_by_name(instanceName);
                if (!instanceSym || instanceSym->type() != zenkit::DaedalusDataType::INSTANCE) {
                    return "";
                }
                
                auto instance = getInstance(instanceSym);
                if (!instance) {
                    return "";
                }
                
                auto* memberSym = findMemberSymbol(instanceSym, symbolName);
                if (!memberSym || !memberSym->is_member() || 
                    memberSym->type() != zenkit::DaedalusDataType::STRING) {
                    return "";
                }
                
                try {
                    std::string result = memberSym->get_string(0, instance.get());
                    // Convert Windows-1250 to UTF-8 for proper display in JavaScript
                    return convertWindows1250ToUtf8(result);
                } catch (...) {
                    return "";
                }
            } else {
                // Looking up a global symbol
                auto* memberSym = vm_.find_symbol_by_name(symbolName);
                if (!memberSym || memberSym->type() != zenkit::DaedalusDataType::STRING) {
                    return "";
                }
                try {
                    std::string result = memberSym->get_string();
                    // Convert Windows-1250 to UTF-8 for proper display in JavaScript
                    return convertWindows1250ToUtf8(result);
                } catch (...) {
                    return "";
                }
            }
        } catch (...) {
            return "";
        }
    }

    int32_t DaedalusVmWrapper::getSymbolInt(const std::string& symbolName, const std::string& instanceName) {
        try {
            if (!instanceName.empty()) {
                auto* instanceSym = vm_.find_symbol_by_name(instanceName);
                if (!instanceSym || instanceSym->type() != zenkit::DaedalusDataType::INSTANCE) {
                    return 0;
                }
                
                auto instance = getInstance(instanceSym);
                if (!instance) {
                    return 0;
                }
                
                auto* memberSym = findMemberSymbol(instanceSym, symbolName);
                if (!memberSym || !memberSym->is_member() || 
                    (memberSym->type() != zenkit::DaedalusDataType::INT && 
                     memberSym->type() != zenkit::DaedalusDataType::FUNCTION)) {
                    return 0;
                }
                
                try {
                    return memberSym->get_int(0, instance.get());
                } catch (...) {
                    return 0;
                }
            } else {
                // Looking up a global symbol
                auto* memberSym = vm_.find_symbol_by_name(symbolName);
                if (!memberSym || (memberSym->type() != zenkit::DaedalusDataType::INT && 
                                  memberSym->type() != zenkit::DaedalusDataType::FUNCTION)) {
                    return 0;
                }
                try {
                    return memberSym->get_int();
                } catch (...) {
                    return 0;
                }
            }
        } catch (...) {
            return 0;
        }
    }

    float DaedalusVmWrapper::getSymbolFloat(const std::string& symbolName, const std::string& instanceName) {
        try {
            if (!instanceName.empty()) {
                zenkit::DaedalusSymbol* instanceSym = vm_.find_symbol_by_name(instanceName);
                if (!instanceSym || instanceSym->type() != zenkit::DaedalusDataType::INSTANCE) {
                    return 0.0f;
                }
                
                std::shared_ptr<zenkit::DaedalusInstance> instance;
                try {
                    instance = instanceSym->get_instance();
                } catch (...) {
                    // Instance might not be initialized yet
                }
                
                if (!instance) {
                    // Instance not initialized - return default value
                    // ZenKit user should ensure instances are initialized before accessing properties
                    return 0.0f;
                }
                
                if (!instance) {
                    return 0.0f;
                }

                zenkit::DaedalusSymbol* memberSym = nullptr;
                try {
                    if (instanceSym->parent() != static_cast<uint32_t>(-1)) {
                        auto* parentSym = vm_.find_symbol_by_index(instanceSym->parent());
                        if (parentSym && parentSym->type() == zenkit::DaedalusDataType::CLASS) {
                            std::string qualifiedName = parentSym->name() + "." + symbolName;
                            memberSym = vm_.find_symbol_by_name(qualifiedName);
                        }
                    }
                } catch (...) {
                    // Parent lookup failed
                }
                
                if (!memberSym) {
                    try {
                        memberSym = vm_.find_symbol_by_name(symbolName);
                    } catch (...) {
                        return 0.0f;
                    }
                }
                
                if (!memberSym || memberSym->type() != zenkit::DaedalusDataType::FLOAT) {
                    return 0.0f;
                }

                try {
                    return memberSym->get_float(0, instance.get());
                } catch (const std::exception& e) {
                    return 0.0f;
                }
            } else {
                auto* memberSym = vm_.find_symbol_by_name(symbolName);
                if (!memberSym || memberSym->type() != zenkit::DaedalusDataType::FLOAT) {
                    return 0.0f;
                }
                try {
                    return memberSym->get_float();
                } catch (...) {
                    return 0.0f;
                }
            }
        } catch (const std::exception& e) {
            return 0.0f;
        } catch (...) {
            return 0.0f;
        }
    }

    // ParamValue struct definition
    struct ParamValue {
        std::variant<int32_t, float, std::string, std::shared_ptr<zenkit::DaedalusInstance>> value;
        zenkit::DaedalusDataType type;
    };

    ParamValue convertJsParam(const emscripten::val& jsVal, zenkit::DaedalusSymbol* paramSym, zenkit::DaedalusVm& vm) {
        ParamValue result;
        result.type = paramSym->type();
        
        if (paramSym->type() == zenkit::DaedalusDataType::INT || 
            paramSym->type() == zenkit::DaedalusDataType::FUNCTION) {
            result.value = jsVal.as<int32_t>();
        } else if (paramSym->type() == zenkit::DaedalusDataType::FLOAT) {
            result.value = jsVal.as<float>();
        } else if (paramSym->type() == zenkit::DaedalusDataType::STRING) {
            result.value = jsVal.as<std::string>();
        } else if (paramSym->type() == zenkit::DaedalusDataType::INSTANCE) {
            std::shared_ptr<zenkit::DaedalusInstance> instance;
            if (jsVal.hasOwnProperty("symbol_index")) {
                int32_t idx = jsVal["symbol_index"].as<int32_t>();
                if (idx >= 0) {
                    auto* instanceSym = vm.find_symbol_by_index(idx);
                    if (instanceSym) {
                        try {
                            instance = instanceSym->get_instance();
                        } catch (...) {
                            // Instance not initialized - return error
                            // ZenKit user should ensure instances are initialized before use
                            instance = nullptr;
                        }
                    }
                }
            } else if (jsVal.typeOf().as<std::string>() == "string") {
                std::string name = jsVal.as<std::string>();
                auto* instanceSym = vm.find_symbol_by_name(name);
                if (instanceSym && instanceSym->type() == zenkit::DaedalusDataType::INSTANCE) {
                    try {
                        instance = instanceSym->get_instance();
                    } catch (...) {
                        // Instance not initialized - return nullptr
                        // ZenKit user should ensure instances are initialized before use
                        instance = nullptr;
                    }
                }
            }
            result.value = instance;
        }
        return result;
    }

    // Helper to convert instance to JS object
    emscripten::val instanceToJs(std::shared_ptr<zenkit::DaedalusInstance> instance, zenkit::DaedalusVm& vm) {
        emscripten::val jsResult = emscripten::val::object();
        if (instance) {
            jsResult.set("symbol_index", emscripten::val(instance->symbol_index()));
            try {
                auto* instanceSym = vm.find_symbol_by_index(instance->symbol_index());
                if (instanceSym) {
                    jsResult.set("name", emscripten::val(instanceSym->name()));
                }
            } catch (...) {}
        } else {
            jsResult.set("symbol_index", emscripten::val(-1));
        }
        return jsResult;
    }

    Result<std::string> DaedalusVmWrapper::getSymbolNameByIndex(int32_t symbolIndex) {
        try {
            if (symbolIndex < 0) {
                return Result<std::string>("Invalid symbol index: " + std::to_string(symbolIndex));
            }
            
            auto* sym = vm_.find_symbol_by_index(static_cast<uint32_t>(symbolIndex));
            if (!sym) {
                return Result<std::string>("Symbol not found at index: " + std::to_string(symbolIndex));
            }
            
            // Explicitly construct string to avoid ambiguity with error constructor
            std::string name = sym->name();
            return Result<std::string>(std::move(name));
        } catch (const std::exception& e) {
            return Result<std::string>("Error getting symbol name: " + std::string(e.what()));
        } catch (...) {
            return Result<std::string>("Unknown error getting symbol name");
        }
    }

    Result<emscripten::val> DaedalusVmWrapper::getInstancePropertyByIndex(int32_t instanceIndex, const std::string& propertyName) {
        try {
            if (instanceIndex < 0) {
                return Result<emscripten::val>("Invalid instance index: " + std::to_string(instanceIndex));
            }
            
            // Get symbol name from index
            auto* instanceSym = vm_.find_symbol_by_index(static_cast<uint32_t>(instanceIndex));
            if (!instanceSym) {
                return Result<emscripten::val>("Instance not found at index: " + std::to_string(instanceIndex));
            }
            
            if (instanceSym->type() != zenkit::DaedalusDataType::INSTANCE) {
                return Result<emscripten::val>("Symbol at index " + std::to_string(instanceIndex) + " is not an instance");
            }
            
            std::string instanceName = instanceSym->name();
            
            // Try to get the property value based on type
            auto* memberSym = findMemberSymbol(instanceSym, propertyName);
            if (!memberSym || !memberSym->is_member()) {
                return Result<emscripten::val>("Property '" + propertyName + "' not found in instance '" + instanceName + "'");
            }
            
            auto instance = getInstance(instanceSym);
            if (!instance) {
                return Result<emscripten::val>("Failed to get or create instance '" + instanceName + "'");
            }
            
            // Get value based on property type
            if (memberSym->type() == zenkit::DaedalusDataType::STRING) {
                try {
                    std::string result = memberSym->get_string(0, instance.get());
                    std::string utf8_result = convertWindows1250ToUtf8(result);
                    return Result<emscripten::val>(emscripten::val(utf8_result));
                } catch (...) {
                    return Result<emscripten::val>("Error reading string property '" + propertyName + "'");
                }
            } else if (memberSym->type() == zenkit::DaedalusDataType::INT || 
                       memberSym->type() == zenkit::DaedalusDataType::FUNCTION) {
                try {
                    int32_t result = memberSym->get_int(0, instance.get());
                    return Result<emscripten::val>(emscripten::val(result));
                } catch (...) {
                    return Result<emscripten::val>("Error reading int property '" + propertyName + "'");
                }
            } else if (memberSym->type() == zenkit::DaedalusDataType::FLOAT) {
                try {
                    float result = memberSym->get_float(0, instance.get());
                    return Result<emscripten::val>(emscripten::val(result));
                } catch (...) {
                    return Result<emscripten::val>("Error reading float property '" + propertyName + "'");
                }
            } else if (memberSym->type() == zenkit::DaedalusDataType::INSTANCE) {
                // For instance properties, we need to get the instance value
                // Note: get_instance() doesn't take context, so we'll need to use VM methods
                // For now, return null instance object
                emscripten::val nullInstance = emscripten::val::object();
                nullInstance.set("symbol_index", emscripten::val(-1));
                return Result<emscripten::val>(std::move(nullInstance));
            } else {
                return Result<emscripten::val>("Unsupported property type for '" + propertyName + "'");
            }
        } catch (const std::exception& e) {
            return Result<emscripten::val>("Error getting instance property: " + std::string(e.what()));
        } catch (...) {
            return Result<emscripten::val>("Unknown error getting instance property");
        }
    }

    Result<emscripten::val> DaedalusVmWrapper::callFunction(const std::string& functionName, const emscripten::val& params) {
        try {
            auto* sym = vm_.find_symbol_by_name(functionName);
            if (sym == nullptr) {
                return Result<emscripten::val>("Function '" + functionName + "' not found");
            }
            
            if (sym->type() != zenkit::DaedalusDataType::FUNCTION) {
                return Result<emscripten::val>("Symbol '" + functionName + "' is not a function");
            }
            
            // Get function signature
            std::vector<zenkit::DaedalusSymbol*> paramSyms = vm_.find_parameters_for_function(sym);
            size_t expectedParamCount = paramSyms.size();
            size_t providedParamCount = params["length"].as<size_t>();
            
            if (providedParamCount != expectedParamCount) {
                return Result<emscripten::val>("Parameter count mismatch: expected " + 
                                              std::to_string(expectedParamCount) + ", got " + 
                                              std::to_string(providedParamCount));
            }
            
            // Convert all parameters
            std::vector<ParamValue> cppParams;
            cppParams.reserve(expectedParamCount);
            for (size_t i = 0; i < expectedParamCount; ++i) {
                emscripten::val jsVal = params[i];
                cppParams.push_back(convertJsParam(jsVal, paramSyms[i], vm_));
            }
            
            // Use manual stack manipulation for all cases (most flexible)
            // Push parameters onto stack in reverse order
            for (int i = static_cast<int>(cppParams.size()) - 1; i >= 0; --i) {
                const auto& p = cppParams[i];
                if (p.type == zenkit::DaedalusDataType::INT || p.type == zenkit::DaedalusDataType::FUNCTION) {
                    vm_.push_int(std::get<int32_t>(p.value));
                } else if (p.type == zenkit::DaedalusDataType::FLOAT) {
                    vm_.push_float(std::get<float>(p.value));
                } else if (p.type == zenkit::DaedalusDataType::STRING) {
                    vm_.push_string(std::get<std::string>(p.value));
                } else if (p.type == zenkit::DaedalusDataType::INSTANCE) {
                    vm_.push_instance(std::get<std::shared_ptr<zenkit::DaedalusInstance>>(p.value));
                }
            }
            
            // Call the function and handle return value
            try {
                vm_.unsafe_call(sym);
                
                // Get return value if needed
                // Note: pop_call() in the VM handles stack cleanup automatically
                // We only need to pop if the function actually has a return value
                if (!sym->has_return()) {
                    // Function is void - no return value to pop
                    return Result<emscripten::val>(emscripten::val::undefined());
                }
                
                // Function has a return value - pop it from the stack
                try {
                    if (sym->rtype() == zenkit::DaedalusDataType::INT) {
                        int32_t result = vm_.pop_int();
                        return Result<emscripten::val>(emscripten::val(result));
                    } else if (sym->rtype() == zenkit::DaedalusDataType::FLOAT) {
                        float result = vm_.pop_float();
                        return Result<emscripten::val>(emscripten::val(result));
                    } else if (sym->rtype() == zenkit::DaedalusDataType::STRING) {
                        std::string result = vm_.pop_string();
                        // Convert Windows-1250 to UTF-8 for proper display in JavaScript
                        std::string utf8_result = convertWindows1250ToUtf8(result);
                        return Result<emscripten::val>(emscripten::val(utf8_result));
                    } else if (sym->rtype() == zenkit::DaedalusDataType::INSTANCE) {
                        auto result = vm_.pop_instance();
                        return Result<emscripten::val>(instanceToJs(result, vm_));
                    }
                    
                    return Result<emscripten::val>("Unsupported return type for function '" + functionName + "' (type: " + 
                                                    std::to_string(static_cast<int>(sym->rtype())) + ")");
                } catch (const zenkit::DaedalusVmException& e) {
                    return Result<emscripten::val>("Error reading return value from '" + functionName + "': " + std::string(e.what()));
                } catch (const zenkit::DaedalusScriptError& e) {
                    return Result<emscripten::val>("Error reading return value from '" + functionName + "': " + std::string(e.what()));
                } catch (const std::exception& e) {
                    return Result<emscripten::val>("Error reading return value from '" + functionName + "': " + std::string(e.what()));
                }
            } catch (const zenkit::DaedalusVmException& e) {
                return Result<emscripten::val>("VM Exception in '" + functionName + "': " + std::string(e.what()));
            } catch (const zenkit::DaedalusScriptError& e) {
                return Result<emscripten::val>("Script Error in '" + functionName + "': " + std::string(e.what()));
            } catch (const std::exception& e) {
                return Result<emscripten::val>("Exception in '" + functionName + "': " + std::string(e.what()));
            } catch (...) {
                return Result<emscripten::val>("Unknown exception in '" + functionName + "'");
            }
        } catch (const zenkit::DaedalusVmException& e) {
            return Result<emscripten::val>("VM Exception in '" + functionName + "': " + std::string(e.what()));
        } catch (const zenkit::DaedalusScriptError& e) {
            return Result<emscripten::val>("Script Error in '" + functionName + "': " + std::string(e.what()));
        } catch (const std::exception& e) {
            return Result<emscripten::val>("Exception in '" + functionName + "': " + std::string(e.what()));
        } catch (...) {
            return Result<emscripten::val>("Unknown error calling function '" + functionName + "'");
        }
    }


    // Helper to get type name as string for error messages
    std::string getTypeName(zenkit::DaedalusDataType type) {
        switch (type) {
            case zenkit::DaedalusDataType::INT: return "int";
            case zenkit::DaedalusDataType::FLOAT: return "float";
            case zenkit::DaedalusDataType::STRING: return "string";
            case zenkit::DaedalusDataType::INSTANCE: return "instance";
            case zenkit::DaedalusDataType::FUNCTION: return "func";
            default: return "unknown";
        }
    }

    // Helper to convert C++ parameter to JS value based on type
    emscripten::val cppParamToJs(zenkit::DaedalusVm& vm, zenkit::DaedalusSymbol* paramSym) {
        zenkit::DaedalusDataType paramType = paramSym->type();
        if (paramType == zenkit::DaedalusDataType::INT || paramType == zenkit::DaedalusDataType::FUNCTION) {
            return emscripten::val(vm.pop_int());
        } else if (paramType == zenkit::DaedalusDataType::FLOAT) {
            return emscripten::val(vm.pop_float());
        } else if (paramType == zenkit::DaedalusDataType::STRING) {
            std::string str = vm.pop_string();
            // Convert Windows-1250 to UTF-8 for proper display in JavaScript
            std::string utf8_str = convertWindows1250ToUtf8(str);
            return emscripten::val(utf8_str);
        } else if (paramType == zenkit::DaedalusDataType::INSTANCE) {
            auto instance = vm.pop_instance();
            return instanceToJs(instance, vm);
        }
        // Fallback: pop as reference (for unknown types)
        (void) vm.pop_reference();
        return emscripten::val::undefined();
    }
    
    // Helper to push JS return value to VM stack
    void DaedalusVmWrapper::pushJsReturnValue(zenkit::DaedalusVm& vm, const emscripten::val& result, zenkit::DaedalusDataType returnType) {
        if (returnType == zenkit::DaedalusDataType::INT || returnType == zenkit::DaedalusDataType::FUNCTION) {
            if (!result.isUndefined()) {
                vm.push_int(result.as<int32_t>());
            } else {
                vm.push_int(0);
            }
        } else if (returnType == zenkit::DaedalusDataType::FLOAT) {
            if (!result.isUndefined()) {
                vm.push_float(result.as<float>());
            } else {
                vm.push_float(0.0f);
            }
        } else if (returnType == zenkit::DaedalusDataType::STRING) {
            if (!result.isUndefined()) {
                std::string str = result.as<std::string>();
                vm.push_string(str);
            } else {
                vm.push_string("");
            }
        } else if (returnType == zenkit::DaedalusDataType::INSTANCE) {
            if (!result.isUndefined() && result.hasOwnProperty("symbol_index")) {
                int32_t idx = result["symbol_index"].as<int32_t>();
                if (idx >= 0) {
                    auto* instanceSym = vm.find_symbol_by_index(idx);
                    if (instanceSym) {
                        auto instance = getInstance(instanceSym);
                        vm.push_instance(instance);
                    } else {
                        vm.push_instance(nullptr);
                    }
                } else {
                    vm.push_instance(nullptr);
                }
            } else {
                vm.push_instance(nullptr);
            }
        }
    }

    void DaedalusVmWrapper::handleUniversalExternal(zenkit::DaedalusVm& vm, zenkit::DaedalusSymbol& sym) {
        std::string funcName = sym.name();
        
        // Try exact match first
        auto it = externalCallbacks_.find(funcName);
        
        // If not found, try case-insensitive lookup (for compatibility)
        if (it == externalCallbacks_.end()) {
            for (const auto& pair : externalCallbacks_) {
                // Case-insensitive comparison
                std::string registeredName = pair.first;
                bool match = true;
                if (registeredName.length() == funcName.length()) {
                    for (size_t i = 0; i < registeredName.length(); ++i) {
                        if (std::tolower(registeredName[i]) != std::tolower(funcName[i])) {
                            match = false;
                            break;
                        }
                    }
                    if (match) {
                        it = externalCallbacks_.find(registeredName);
                        break;
                    }
                }
            }
        }
        
        if (it == externalCallbacks_.end()) {
            // Not registered - print helpful error message and use default handler behavior
            std::cerr << "⚠️  VM: External function '" << funcName << "' is not implemented (called but not registered)" << std::endl;
            
            // Get parameter types for better error message
            auto params = vm.find_parameters_for_function(&sym);
            if (!params.empty()) {
                std::cerr << "   Signature: ";
                if (sym.has_return()) {
                    std::cerr << getTypeName(sym.rtype()) << " ";
                } else {
                    std::cerr << "void ";
                }
                std::cerr << funcName << "(";
                for (size_t i = 0; i < params.size(); ++i) {
                    if (i > 0) std::cerr << ", ";
                    std::cerr << getTypeName(params[i]->type());
                }
                std::cerr << ")" << std::endl;
            }
            std::cerr << "   To implement, call: vm.registerExternal('" << funcName << "', callback)" << std::endl;
            
            // Pop parameters and push default return value
            for (int i = static_cast<int>(params.size()) - 1; i >= 0; --i) {
                auto* par = params[static_cast<unsigned>(i)];
                if (par->type() == zenkit::DaedalusDataType::INT) {
                    (void) vm.pop_int();
                } else if (par->type() == zenkit::DaedalusDataType::FLOAT) {
                    (void) vm.pop_float();
                } else if (par->type() == zenkit::DaedalusDataType::INSTANCE || par->type() == zenkit::DaedalusDataType::STRING) {
                    (void) vm.pop_reference();
                }
            }
            if (sym.has_return()) {
                if (sym.rtype() == zenkit::DaedalusDataType::FLOAT) {
                    vm.push_float(0.0f);
                } else if (sym.rtype() == zenkit::DaedalusDataType::INT) {
                    vm.push_int(0);
                } else if (sym.rtype() == zenkit::DaedalusDataType::STRING) {
                    vm.push_string("");
                } else if (sym.rtype() == zenkit::DaedalusDataType::INSTANCE) {
                    vm.push_instance(nullptr);
                }
            }
            return;
        }
        
        const auto& info = it->second;
        try {
            // Pop parameters from stack in reverse order (as they were pushed)
            std::vector<emscripten::val> jsParams;
            jsParams.reserve(info.params.size());
            
            for (int i = static_cast<int>(info.params.size()) - 1; i >= 0; --i) {
                auto* paramSym = info.params[static_cast<size_t>(i)];
                jsParams.push_back(cppParamToJs(vm, paramSym));
            }
            
            // Reverse to get correct order for JS callback
            std::reverse(jsParams.begin(), jsParams.end());
            
            // Call JavaScript callback with converted parameters using apply (universal for any number of params)
            emscripten::val jsArgs = emscripten::val::array();
            for (size_t i = 0; i < jsParams.size(); ++i) {
                jsArgs.call<void>("push", jsParams[i]);
            }
            emscripten::val result = info.callback.call<emscripten::val>("apply", emscripten::val::null(), jsArgs);
            
            // Handle return value if function has one
            if (info.sym->has_return()) {
                pushJsReturnValue(vm, result, info.sym->rtype());
            }
        } catch (const std::exception& e) {
            std::cerr << "Error in external callback for " << funcName << ": " << e.what() << std::endl;
            // Push default return value if needed
            if (info.sym->has_return()) {
                pushJsReturnValue(vm, emscripten::val::undefined(), info.sym->rtype());
            }
        } catch (...) {
            std::cerr << "Unknown error in external callback for " << funcName << std::endl;
            // Push default return value if needed
            if (info.sym->has_return()) {
                pushJsReturnValue(vm, emscripten::val::undefined(), info.sym->rtype());
            }
        }
    }

    Result<bool> DaedalusVmWrapper::registerExternal(const std::string& functionName, const emscripten::val& callback) {
        try {
            auto* sym = vm_.find_symbol_by_name(functionName);
            if (sym == nullptr) {
                return Result<bool>("External function '" + functionName + "' not found");
            }

            if (!sym->is_external()) {
                return Result<bool>("Symbol '" + functionName + "' is not an external function");
            }

            // Get parameter types to determine signature
            std::vector<zenkit::DaedalusSymbol*> params = vm_.find_parameters_for_function(sym);
            
            // Universal approach: Store callback info in member map and use default external handler
            // to route to the correct callback based on symbol name
            ExternalCallbackInfo info;
            info.callback = callback;
            info.params = params;
            info.sym = sym;
            externalCallbacks_[functionName] = info;
            
            // Set up default external handler on first registration (if not already set)
            if (!defaultExternalHandlerSet_) {
                vm_.register_default_external_custom([this](zenkit::DaedalusVm& vm, zenkit::DaedalusSymbol& sym) {
                    this->handleUniversalExternal(vm, sym);
                });
                defaultExternalHandlerSet_ = true;
            }
            
            return Result<bool>(true);
            
        } catch (const std::exception& e) {
            return Result<bool>(e.what());
        } catch (...) {
            return Result<bool>("Unknown error registering external function " + functionName);
        }
    }

    // Helper function to convert JS instance parameter to C++ instance
    Result<std::shared_ptr<zenkit::DaedalusInstance>> DaedalusVmWrapper::parseInstanceParameter(const emscripten::val& instanceName) {
        try {
            std::shared_ptr<zenkit::DaedalusInstance> instance;
            
            // Handle instance name as string or object with symbol_index
            if (instanceName.typeOf().as<std::string>() == "string") {
                std::string name = instanceName.as<std::string>();
                auto* instanceSym = vm_.find_symbol_by_name(name);
                if (!instanceSym || instanceSym->type() != zenkit::DaedalusDataType::INSTANCE) {
                    return Result<std::shared_ptr<zenkit::DaedalusInstance>>("Instance '" + name + "' not found or not an instance type");
                }
                instance = getInstance(instanceSym);
            } else if (instanceName.hasOwnProperty("symbol_index")) {
                int32_t idx = instanceName["symbol_index"].as<int32_t>();
                if (idx >= 0) {
                    auto* instanceSym = vm_.find_symbol_by_index(idx);
                    if (instanceSym) {
                        instance = getInstance(instanceSym);
                    }
                }
            } else {
                return Result<std::shared_ptr<zenkit::DaedalusInstance>>("Invalid instance parameter: expected string or object with symbol_index");
            }
            
            if (!instance) {
                return Result<std::shared_ptr<zenkit::DaedalusInstance>>("Failed to get or create instance");
            }
            
            return Result<std::shared_ptr<zenkit::DaedalusInstance>>(std::move(instance));
        } catch (const std::exception& e) {
            return Result<std::shared_ptr<zenkit::DaedalusInstance>>(e.what());
        } catch (...) {
            return Result<std::shared_ptr<zenkit::DaedalusInstance>>("Unknown error parsing instance parameter");
        }
    }

    Result<bool> DaedalusVmWrapper::setGlobalSelf(const emscripten::val& instanceName) {
        try {
            auto* globalSelfSym = vm_.global_self();
            if (!globalSelfSym) {
                return Result<bool>("Global 'self' symbol not found in VM");
            }
            
            auto instanceResult = parseInstanceParameter(instanceName);
            if (!instanceResult.success) {
                return Result<bool>(instanceResult.error_message);
            }
            
            globalSelfSym->set_instance(instanceResult.data);
            return Result<bool>(true);
        } catch (const std::exception& e) {
            return Result<bool>(e.what());
        } catch (...) {
            return Result<bool>("Unknown error setting global 'self'");
        }
    }

    Result<bool> DaedalusVmWrapper::setGlobalOther(const emscripten::val& instanceName) {
        try {
            auto* globalOtherSym = vm_.global_other();
            if (!globalOtherSym) {
                return Result<bool>("Global 'other' symbol not found in VM");
            }
            
            auto instanceResult = parseInstanceParameter(instanceName);
            if (!instanceResult.success) {
                return Result<bool>(instanceResult.error_message);
            }
            
            globalOtherSym->set_instance(instanceResult.data);
            return Result<bool>(true);
        } catch (const std::exception& e) {
            return Result<bool>(e.what());
        } catch (...) {
            return Result<bool>("Unknown error setting global 'other'");
        }
    }

    Result<emscripten::val> DaedalusVmWrapper::initInstanceByIndex(int32_t symbolIndex) {
        try {
            if (symbolIndex < 0) {
                return Result<emscripten::val>("Invalid symbol index: " + std::to_string(symbolIndex));
            }
            
            auto* instanceSym = vm_.find_symbol_by_index(static_cast<uint32_t>(symbolIndex));
            if (!instanceSym) {
                return Result<emscripten::val>("Symbol not found at index: " + std::to_string(symbolIndex));
            }
            
            if (instanceSym->type() != zenkit::DaedalusDataType::INSTANCE) {
                return Result<emscripten::val>("Symbol at index " + std::to_string(symbolIndex) + " is not an instance type");
            }
            
            // Check if instance already exists
            std::shared_ptr<zenkit::DaedalusInstance> instance;
            try {
                instance = instanceSym->get_instance();
            } catch (...) {
                // Instance doesn't exist, create it
            }
            
            if (!instance) {
                try {
                    instance = vm_.init_opaque_instance(instanceSym);
                } catch (const std::exception& e) {
                    return Result<emscripten::val>("Failed to initialize instance: " + std::string(e.what()));
                } catch (...) {
                    return Result<emscripten::val>("Unknown error initializing instance");
                }
            }
            
            // Convert instance to JS object
            emscripten::val jsInstance = instanceToJs(instance, vm_);
            return Result<emscripten::val>(std::move(jsInstance));
        } catch (const std::exception& e) {
            return Result<emscripten::val>("Error initializing instance: " + std::string(e.what()));
        } catch (...) {
            return Result<emscripten::val>("Unknown error initializing instance");
        }
    }

    // CutsceneLibraryWrapper implementation
    Result<bool> CutsceneLibraryWrapper::loadFromArray(const emscripten::val& uint8_array, int game_version) {
        try {
            auto reader = create_reader_from_js_array(uint8_array);
            auto archive = zenkit::ReadArchive::from(reader.get());
            
            zenkit::GameVersion version = game_version == 1 
                ? zenkit::GameVersion::GOTHIC_1 
                : zenkit::GameVersion::GOTHIC_2;
            
            // Read the zCCSLib object from the archive
            auto lib = archive->read_object<zenkit::CutsceneLibrary>(version);
            library_ = *lib;
            return Result<bool>(true);
        } catch (const std::exception& e) {
            last_error_ = e.what();
            return Result<bool>(e.what());
        } catch (...) {
            last_error_ = "Unknown error loading cutscene library";
            return Result<bool>(last_error_);
        }
    }

    emscripten::val CutsceneLibraryWrapper::getBlockByName(const std::string& name) const {
        try {
            auto block = library_.block_by_name(name);
            if (!block) {
                return emscripten::val::null();
            }
            
            auto message = block->get_message();
            if (!message) {
                return emscripten::val::null();
            }
            
            // Convert Windows-1250 encoded strings to UTF-8 for proper display in JavaScript
            std::string utf8_text = convertWindows1250ToUtf8(message->text);
            std::string utf8_name = convertWindows1250ToUtf8(message->name);
            std::string utf8_blockName = convertWindows1250ToUtf8(block->name);
            
            emscripten::val result = emscripten::val::object();
            result.set("text", emscripten::val(utf8_text));
            result.set("name", emscripten::val(utf8_name));
            result.set("blockName", emscripten::val(utf8_blockName));
            return result;
        } catch (const std::exception& e) {
            last_error_ = e.what();
            return emscripten::val::null();
        } catch (...) {
            return emscripten::val::null();
        }
    }

    // ModelScriptWrapper implementation
    Result<bool> ModelScriptWrapper::loadFromArray(const emscripten::val& uint8_array) {
        try {
            auto reader = create_reader_from_js_array(uint8_array);
            script_.load(reader.get());
            last_error_.clear();
            return Result<bool>(true);
        } catch (const std::exception& e) {
            last_error_ = e.what();
            return Result<bool>(e.what());
        }
    }

} // namespace zenkit::wasm
