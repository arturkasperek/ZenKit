// Copyright © 2024 GothicKit Contributors.
// SPDX-License-Identifier: MIT
#pragma once

#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <memory>
#include <string>
#include <vector>
#include <cstring>

#include "zenkit/Stream.hh"
#include "zenkit/Misc.hh"
#include "zenkit/Mesh.hh"
#include "zenkit/MultiResolutionMesh.hh"
#include "zenkit/Archive.hh"
#include "zenkit/Texture.hh"
#include "zenkit/World.hh"
#include "zenkit/vobs/VirtualObject.hh"

namespace zenkit::wasm {

    /// \brief Result wrapper for WebAssembly operations
    template<typename T>
    struct Result {
        T data;
        std::string error_message;
        bool success = false;

        Result() = default;
        explicit Result(T&& value) : data(std::move(value)), success(true) {}
        explicit Result(const std::string& error) : error_message(error), success(false) {}
    };

    /// \brief Memory management helper for efficient data transfer
    class DataBuffer {
    public:
        explicit DataBuffer(uintptr_t ptr, size_t size)
            : data_(reinterpret_cast<const std::byte*>(ptr)), size_(size) {}

        [[nodiscard]] const std::byte* data() const { return data_; }
        [[nodiscard]] size_t size() const { return size_; }

    private:
        const std::byte* data_;
        size_t size_;
    };

    /// \brief Factory for creating Read streams from WebAssembly data
    std::unique_ptr<zenkit::Read> create_reader_from_buffer(uintptr_t data_ptr, size_t length);
    std::unique_ptr<zenkit::Read> create_reader_from_string(const std::string& buffer);
    std::unique_ptr<zenkit::Read> create_reader_from_js_array(const emscripten::val& uint8_array);

    // Vector and geometric wrapper classes
    struct Vector3 {
        float x, y, z;
        Vector3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
        Vector3(const zenkit::Vec3& v) : x(v.x), y(v.y), z(v.z) {}
    };

    struct Vector2 {
        float x, y;
        Vector2(float x = 0, float y = 0) : x(x), y(y) {}
        Vector2(const zenkit::Vec2& v) : x(v.x), y(v.y) {}
    };

    struct VertexFeature {
        Vector2 texture;
        uint32_t light;
        Vector3 normal;

        VertexFeature() = default;
        VertexFeature(const zenkit::VertexFeature& feature)
            : texture(feature.texture)
            , light(feature.light)
            , normal(feature.normal) {}
    };

    struct MaterialData {
        std::string name;
        uint8_t group;
        std::string texture;

        MaterialData() = default;
        MaterialData(const zenkit::Material& material)
            : name(material.name)
            , group(static_cast<uint8_t>(material.group))
            , texture(material.texture) {}
    };

    /// \brief Processed mesh data matching OpenGothic's PackedMesh pipeline
    /// This struct contains mesh data after applying material deduplication,
    /// composite vertex processing, and triangle sorting.
    struct ProcessedMeshData {
        std::vector<float> vertices;          // [x,y,z, nx,ny,nz, u,v, ...] per vertex (8 floats per vertex)
        std::vector<uint32_t> indices;        // triangle indices into vertices array
        std::vector<uint32_t> materialIds;    // per-triangle material ID (deduplicated)
        std::vector<MaterialData> materials;  // deduplicated material list
        
        ProcessedMeshData() = default;
    };

    struct OrientedBoundingBoxData {
        Vector3 center;
        std::vector<Vector3> axes;
        Vector3 half_width;

        OrientedBoundingBoxData() = default;
        OrientedBoundingBoxData(const zenkit::OrientedBoundingBox& obb)
            : center(obb.center)
            , half_width(obb.half_width) {
            axes.reserve(3);
            for (int i = 0; i < 3; ++i) {
                axes.emplace_back(obb.axes[i]);
            }
        }
    };

    struct ColorData {
        uint8_t r, g, b, a;

        ColorData() = default;
        ColorData(const zenkit::Color& color) : r(color.r), g(color.g), b(color.b), a(color.a) {}
    };

    struct ArchiveObjectData {
        std::string object_name;
        std::string class_name;
        uint16_t version;
        uint32_t index;

        ArchiveObjectData() = default;
        ArchiveObjectData(const zenkit::ArchiveObject& obj)
            : object_name(obj.object_name)
            , class_name(obj.class_name)
            , version(obj.version)
            , index(obj.index) {}
    };

    struct Matrix3x3Data {
        float data[9]; // 3x3 matrix stored as flat array

        Matrix3x3Data() = default;
        Matrix3x3Data(const zenkit::Mat3& mat) {
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    data[i * 3 + j] = mat[i][j];
                }
            }
        }

        float get(int row, int col) const {
            return data[row * 3 + col];
        }
        
        float getIndex(int index) const {
            return data[index];
        }
        
        std::vector<float> toArray() const {
            return std::vector<float>(data, data + 9);
        }
    };

    /// \brief Visual data for a VOB
    struct VisualData {
        std::string name;           // Mesh filename (e.g., "BEDNAME.3DS")
        uint32_t type;              // VisualType enum value
        
        VisualData() = default;
        VisualData(const zenkit::Visual& visual)
            : name(visual.name)
            , type(static_cast<uint32_t>(visual.type)) {}
    };

    /// \brief VOB (Visual Object) data - represents interactive/static objects in the world
    struct VobData {
        uint32_t id;                // Unique VOB ID
        std::string vob_name;       // VOB name
        uint32_t type;              // VirtualObjectType enum value
        Vector3 position;           // World position
        Matrix3x3Data rotation;     // Rotation matrix
        VisualData visual;          // Visual information (mesh name, type)
        bool show_visual;           // Whether to render the visual
        bool cd_dynamic;            // Collision detection enabled
        std::vector<VobData> children; // Child VOBs
        
        VobData() = default;
        VobData(const zenkit::VirtualObject& vob);  // Forward declaration, implemented in .cc
    };

    struct BoundingBoxData {
        Vector3 min;
        Vector3 max;

        BoundingBoxData() = default;
        BoundingBoxData(const zenkit::AxisAlignedBoundingBox& bbox)
            : min(bbox.min), max(bbox.max) {}
    };

    struct RawDataResult {
        std::vector<uint8_t> data;
        size_t position = 0;
        
        uint8_t read_ubyte() {
            if (position >= data.size()) return 0;
            return data[position++];
        }
    };

    // ReadArchive wrapper for WebAssembly
    class ReadArchiveWrapper {
    public:
        explicit ReadArchiveWrapper(std::unique_ptr<zenkit::ReadArchive> archive)
            : archive_(std::move(archive)) {}

        // Object reading
        bool read_object_begin(ArchiveObjectData& obj) {
            zenkit::ArchiveObject archive_obj;
            bool result = archive_->read_object_begin(archive_obj);
            if (result) {
                obj = ArchiveObjectData(archive_obj);
            }
            return result;
        }

        bool read_object_end() {
            return archive_->read_object_end();
        }

        // Basic data reading
        std::string read_string() {
            return archive_->read_string();
        }

        int32_t read_int() {
            return archive_->read_int();
        }

        float read_float() {
            return archive_->read_float();
        }

        uint8_t read_byte() {
            return archive_->read_byte();
        }

        uint16_t read_word() {
            return archive_->read_word();
        }

        uint32_t read_enum() {
            return archive_->read_enum();
        }

        bool read_bool() {
            return archive_->read_bool();
        }

        // Complex data reading
        ColorData read_color() {
            return ColorData(archive_->read_color());
        }

        Vector3 read_vec3() {
            return Vector3(archive_->read_vec3());
        }

        Vector2 read_vec2() {
            return Vector2(archive_->read_vec2());
        }

        BoundingBoxData read_bbox() {
            return BoundingBoxData(archive_->read_bbox());
        }

        Matrix3x3Data read_mat3x3() {
            return Matrix3x3Data(archive_->read_mat3x3());
        }

        RawDataResult read_raw(size_t size) {
            auto raw_reader = archive_->read_raw(size);
            RawDataResult result;
            result.data.reserve(size);
            for (size_t i = 0; i < size; ++i) {
                result.data.push_back(raw_reader->read_ubyte());
            }
            return result;
        }

        // Skip functionality
        void skip_object(bool skip_current) {
            archive_->skip_object(skip_current);
        }

    private:
        std::unique_ptr<zenkit::ReadArchive> archive_;
    };

    // Factory function for creating ReadArchive
    std::unique_ptr<ReadArchiveWrapper> create_read_archive(uintptr_t data_ptr, size_t length);
    std::unique_ptr<ReadArchiveWrapper> create_read_archive_from_js_array(const emscripten::val& uint8_array);

    // MeshWrapper class - shared between world and mesh bindings
    class MeshWrapper {
    public:
        explicit MeshWrapper(const zenkit::Mesh& mesh) : mesh_(mesh) {}

        // Safe vertex access with bounds checking
        std::vector<Vector3> getVertices() const {
            std::vector<Vector3> positions;
            positions.reserve(mesh_.vertices.size());
            for (const auto& vertex : mesh_.vertices) {
                positions.emplace_back(vertex.x, vertex.y, vertex.z);
            }
            return positions;
        }

        // Safe feature access with bounds checking
        std::vector<VertexFeature> getFeatures() const {
            std::vector<VertexFeature> features;
            features.reserve(mesh_.features.size());
            for (const auto& feature : mesh_.features) {
                features.emplace_back(feature);
            }
            return features;
        }

        std::vector<uint32_t> getVertexIndices() const {
            return mesh_.polygon_vertex_indices;
        }

        // Individual feature components with safety checks
        std::vector<Vector3> getNormals() const {
            std::vector<Vector3> normals;
            normals.reserve(mesh_.features.size());
            for (const auto& feature : mesh_.features) {
                normals.emplace_back(feature.normal);
            }
            return normals;
        }

        std::vector<Vector2> getTextureCoords() const {
            std::vector<Vector2> uvs;
            uvs.reserve(mesh_.features.size());
            for (const auto& feature : mesh_.features) {
                uvs.emplace_back(feature.texture);
            }
            return uvs;
        }

        std::vector<uint32_t> getLightValues() const {
            std::vector<uint32_t> lightValues;
            lightValues.reserve(mesh_.features.size());
            for (const auto& feature : mesh_.features) {
                lightValues.push_back(feature.light);
            }
            return lightValues;
        }

        // Fixed bounding box calculation - ensure proper values are returned
        Vector3 getBoundingBoxMin() const {
            // If bounding box is not properly initialized, calculate it
            if (mesh_.bbox.min.x == 0 && mesh_.bbox.min.y == 0 && mesh_.bbox.min.z == 0 &&
                mesh_.bbox.max.x == 0 && mesh_.bbox.max.y == 0 && mesh_.bbox.max.z == 0) {
                return calculateBoundingBoxMin();
            }
            return Vector3(mesh_.bbox.min);
        }

        Vector3 getBoundingBoxMax() const {
            // If bounding box is not properly initialized, calculate it
            if (mesh_.bbox.min.x == 0 && mesh_.bbox.min.y == 0 && mesh_.bbox.min.z == 0 &&
                mesh_.bbox.max.x == 0 && mesh_.bbox.max.y == 0 && mesh_.bbox.max.z == 0) {
                return calculateBoundingBoxMax();
            }
            return Vector3(mesh_.bbox.max);
        }

        // Materials
        std::vector<MaterialData> getMaterials() const {
            std::vector<MaterialData> materials;
            materials.reserve(mesh_.materials.size());
            for (const auto& material : mesh_.materials) {
                materials.emplace_back(material);
            }
            return materials;
        }

        // Oriented Bounding Box
        OrientedBoundingBoxData getOrientedBoundingBox() const {
            return OrientedBoundingBoxData(mesh_.obb);
        }

        // Basic info (for debugging)
        std::string getName() const { return mesh_.name; }

        // Safe vertex count accessor
        size_t getVertexCount() const { return mesh_.vertices.size(); }
        size_t getFeatureCount() const { return mesh_.features.size(); }
        size_t getIndexCount() const { return mesh_.polygon_vertex_indices.size(); }

        // Performance optimization: Direct typed array access for WebGL
        // IMPORTANT: Return JS-owned TypedArrays, not views into WASM memory.
        emscripten::val getVerticesTypedArray() const {
            if (mesh_.vertices.empty()) {
                return emscripten::val::null();
            }

            // Create a flat array of floats: [x1,y1,z1, x2,y2,z2, ...]
            std::vector<float> flat_vertices;
            flat_vertices.reserve(mesh_.vertices.size() * 3);
            for (const auto& vertex : mesh_.vertices) {
                flat_vertices.push_back(vertex.x);
                flat_vertices.push_back(vertex.y);
                flat_vertices.push_back(vertex.z);
            }

            // Allocate JS Float32Array and copy data into it
            emscripten::val Float32Array = emscripten::val::global("Float32Array");
            emscripten::val js_array = Float32Array.new_(flat_vertices.size());
            js_array.call<void>("set", emscripten::val(emscripten::typed_memory_view(flat_vertices.size(), flat_vertices.data())));
            return js_array;
        }

        emscripten::val getNormalsTypedArray() const {
            if (mesh_.features.empty()) {
                return emscripten::val::null();
            }

            std::vector<float> flat_normals;
            flat_normals.reserve(mesh_.features.size() * 3);
            for (const auto& feature : mesh_.features) {
                flat_normals.push_back(feature.normal.x);
                flat_normals.push_back(feature.normal.y);
                flat_normals.push_back(feature.normal.z);
            }

            emscripten::val Float32Array = emscripten::val::global("Float32Array");
            emscripten::val js_array = Float32Array.new_(flat_normals.size());
            js_array.call<void>("set", emscripten::val(emscripten::typed_memory_view(flat_normals.size(), flat_normals.data())));
            return js_array;
        }

        emscripten::val getUVsTypedArray() const {
            if (mesh_.features.empty()) {
                return emscripten::val::null();
            }

            std::vector<float> flat_uvs;
            flat_uvs.reserve(mesh_.features.size() * 2);
            for (const auto& feature : mesh_.features) {
                flat_uvs.push_back(feature.texture.x);
                flat_uvs.push_back(feature.texture.y);
            }

            emscripten::val Float32Array = emscripten::val::global("Float32Array");
            emscripten::val js_array = Float32Array.new_(flat_uvs.size());
            js_array.call<void>("set", emscripten::val(emscripten::typed_memory_view(flat_uvs.size(), flat_uvs.data())));
            return js_array;
        }

        emscripten::val getIndicesTypedArray() const {
            // Only expose triangulated indices suitable for GL_TRIANGLES
            if (mesh_.polygons.vertex_indices.empty()) {
                return emscripten::val::null();
            }

            emscripten::val Uint32Array = emscripten::val::global("Uint32Array");
            emscripten::val js_array = Uint32Array.new_(mesh_.polygons.vertex_indices.size());
            js_array.call<void>(
                "set",
                emscripten::val(emscripten::typed_memory_view(
                    mesh_.polygons.vertex_indices.size(),
                    mesh_.polygons.vertex_indices.data()
                ))
            );
            return js_array;
        }

        emscripten::val getFeatureIndicesTypedArray() const {
            if (mesh_.polygons.feature_indices.empty()) {
                return emscripten::val::null();
            }
            emscripten::val Uint32Array = emscripten::val::global("Uint32Array");
            emscripten::val js_array = Uint32Array.new_(mesh_.polygons.feature_indices.size());
            js_array.call<void>(
                "set",
                emscripten::val(emscripten::typed_memory_view(
                    mesh_.polygons.feature_indices.size(),
                    mesh_.polygons.feature_indices.data()
                ))
            );
            return js_array;
        }

        emscripten::val getTriFeatureIndicesTypedArray() const {
            if (mesh_.polygon_feature_indices.empty()) {
                return emscripten::val::null();
            }
            emscripten::val Uint32Array = emscripten::val::global("Uint32Array");
            emscripten::val js_array = Uint32Array.new_(mesh_.polygon_feature_indices.size());
            js_array.call<void>(
                "set",
                emscripten::val(emscripten::typed_memory_view(
                    mesh_.polygon_feature_indices.size(),
                    mesh_.polygon_feature_indices.data()
                ))
            );
            return js_array;
        }

        // Material index per triangle (aligned with triangles in vertex_indices/3)
        emscripten::val getPolygonMaterialIndicesTypedArray() const {
            if (mesh_.polygons.material_indices.empty()) {
                return emscripten::val::null();
            }

            emscripten::val Uint32Array = emscripten::val::global("Uint32Array");
            emscripten::val js_array = Uint32Array.new_(mesh_.polygons.material_indices.size());
            js_array.call<void>(
                "set",
                emscripten::val(emscripten::typed_memory_view(
                    mesh_.polygons.material_indices.size(),
                    mesh_.polygons.material_indices.data()
                ))
            );
            return js_array;
        }

        /// \brief Get processed mesh data matching OpenGothic's PackedMesh pipeline
        /// This applies material deduplication, composite vertex processing (vertex+feature indices),
        /// the feature index bit-shift fix, and triangle sorting by material.
        ProcessedMeshData getProcessedMeshData() const;

    private:
        const zenkit::Mesh& mesh_;

        // Helper methods for bounding box calculation
        Vector3 calculateBoundingBoxMin() const {
            if (mesh_.vertices.empty()) {
                return Vector3(0, 0, 0);
            }

            zenkit::Vec3 min = mesh_.vertices[0];
            for (const auto& vertex : mesh_.vertices) {
                min.x = std::min(min.x, vertex.x);
                min.y = std::min(min.y, vertex.y);
                min.z = std::min(min.z, vertex.z);
            }
            return Vector3(min);
        }

        Vector3 calculateBoundingBoxMax() const {
            if (mesh_.vertices.empty()) {
                return Vector3(0, 0, 0);
            }

            zenkit::Vec3 max = mesh_.vertices[0];
            for (const auto& vertex : mesh_.vertices) {
                max.x = std::max(max.x, vertex.x);
                max.y = std::max(max.y, vertex.y);
                max.z = std::max(max.z, vertex.z);
            }
            return Vector3(max);
        }

        /// \brief Helper method to check if two materials are visually identical (for deduplication)
        static bool isVisuallySame(const zenkit::Material& a, const zenkit::Material& b);
    };

    // Forward declaration for World wrapper
    class WorldWrapper;

    // Texture wrapper for exposing TEX -> RGBA8 to JS
    class TextureWrapper {
    public:
        TextureWrapper() = default;
        ~TextureWrapper() = default;

        Result<bool> loadFromArray(const emscripten::val& uint8_array);

        [[nodiscard]] uint32_t width()   const { return tex_.width(); }
        [[nodiscard]] uint32_t height()  const { return tex_.height(); }
        [[nodiscard]] uint32_t mipmaps() const { return tex_.mipmaps(); }

        // Returns JS Uint8Array of RGBA8 pixels for the requested mip level
        emscripten::val asRgba8(uint32_t mip_level) const;

    private:
        zenkit::Texture tex_;
    };

    // Standalone Mesh wrapper for loading mesh files
    class StandaloneMeshWrapper {
    public:
        StandaloneMeshWrapper() = default;
        ~StandaloneMeshWrapper() = default;

        Result<bool> loadFromArray(const emscripten::val& uint8_array);
        Result<bool> loadMRMFromArray(const emscripten::val& uint8_array);

        // Get mesh wrapper for accessing mesh data
        std::unique_ptr<MeshWrapper> getMeshData() const {
            return std::make_unique<MeshWrapper>(mesh_);
        }
        
        bool isMRM() const { return is_mrm_; }

    private:
        zenkit::Mesh mesh_;
        zenkit::MultiResolutionMesh mrm_;
        bool is_mrm_ = false;
    };

} // namespace zenkit::wasm