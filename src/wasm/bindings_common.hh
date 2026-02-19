// Copyright © 2024 GothicKit Contributors.
// SPDX-License-Identifier: MIT
#pragma once

#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstring>

#include "zenkit/Stream.hh"
#include "zenkit/Misc.hh"
#include "zenkit/Mesh.hh"
#include "zenkit/Model.hh"
#include "zenkit/MultiResolutionMesh.hh"
#include "zenkit/SoftSkinMesh.hh"
#include "zenkit/MorphMesh.hh"
#include "zenkit/Archive.hh"
#include "zenkit/Texture.hh"
#include "zenkit/World.hh"
#include "zenkit/vobs/VirtualObject.hh"
#include "zenkit/DaedalusScript.hh"
#include "zenkit/DaedalusVm.hh"
#include "zenkit/addon/daedalus.hh"
#include "zenkit/CutsceneLibrary.hh"
#include "zenkit/ModelScript.hh"
#include "zenkit/ModelAnimation.hh"

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
        bool disable_collision = false;

        MaterialData() = default;
        MaterialData(const zenkit::Material& material)
            : name(material.name)
            , group(static_cast<uint8_t>(material.group))
            , texture(material.texture)
            , disable_collision(material.disable_collision) {}
    };

    /// \brief Processed mesh data matching OpenGothic's PackedMesh pipeline
    /// This struct contains mesh data after applying material deduplication,
    /// composite vertex processing, and triangle sorting.
    struct ProcessedMeshData {
        std::vector<float> vertices;          // [x,y,z, nx,ny,nz, u,v, ...] per vertex (8 floats per vertex)
        std::vector<uint32_t> indices;        // triangle indices into vertices array
        std::vector<uint32_t> materialIds;    // per-triangle material ID (deduplicated)
        std::vector<MaterialData> materials;  // deduplicated material list
        std::vector<float> boneWeights;       // 4 weights per vertex
        std::vector<uint32_t> boneIndices;    // 4 bone indices per vertex
        std::vector<float> bonePositions;     // 4 * vec3 (pos0..3) per vertex, OpenGothic-style

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

    /// \brief Represents a 4x4 matrix for transformation data
    struct Matrix4x4Data {
        float data[16]; // 4x4 matrix stored as flat array in column-major order

        Matrix4x4Data() = default;
        Matrix4x4Data(const zenkit::Mat4& mat) {
            for (int col = 0; col < 4; ++col) {
                for (int row = 0; row < 4; ++row) {
                    data[col * 4 + row] = mat[col][row];
                }
            }
        }

        // Method to get elements by index (column-major)
        float get(size_t index) const {
            if (index >= 16) {
                return 0.0f;
            }
            return data[index];
        }

        // Method to get elements by row and column
        float get(size_t row, size_t col) const {
            if (row >= 4 || col >= 4) {
                return 0.0f;
            }
            return data[col * 4 + row];
        }

        // Method to convert to a JavaScript array (column-major)
        emscripten::val toArray() const {
            emscripten::val js_array = emscripten::val::array();
            for (int i = 0; i < 16; ++i) {
                js_array.call<void>("push", data[i]);
            }
            return js_array;
        }

        // Method to get the size of the underlying array
        size_t size() const {
            return 16;
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

    struct BoundingBoxData {
        Vector3 min;
        Vector3 max;

        BoundingBoxData() = default;
        BoundingBoxData(const zenkit::AxisAlignedBoundingBox& bbox)
            : min(bbox.min), max(bbox.max) {}
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
        bool cd_static;             // Static collision detection enabled
        bool cd_dynamic;            // Dynamic collision detection enabled
        bool vob_static;            // VOB is static
        bool physics_enabled;       // VOB has physics enabled
        BoundingBoxData bbox;       // Axis-aligned bounding box (world space)
        std::vector<VobData> children; // Child VOBs
        
        VobData() = default;
        VobData(const zenkit::VirtualObject& vob);  // Forward declaration, implemented in .cc
    };

    /// \brief Waypoint data for navigation/pathfinding
    struct WayPointData {
        std::string name;
        Vector3 position;
        Vector3 direction;
        int32_t water_depth;
        bool under_water;
        bool free_point;

        WayPointData() = default;
    };

    /// \brief Waypoint edge (connection between two waypoints)
    struct WayEdgeData {
        uint32_t waypoint_a_index;
        uint32_t waypoint_b_index;

        WayEdgeData() = default;
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

    /// \brief WebAssembly wrapper for zenkit::Model that provides access to model data
    class ModelWrapper {
    public:
        ModelWrapper() = default;
        explicit ModelWrapper(const zenkit::Model& model) : model_(model) {}
        ~ModelWrapper() = default;

        // Non-copyable but movable
        ModelWrapper(const ModelWrapper&) = delete;
        ModelWrapper& operator=(const ModelWrapper&) = delete;
        ModelWrapper(ModelWrapper&&) = default;
        ModelWrapper& operator=(ModelWrapper&&) = default;

        /// \brief Load model from WebAssembly memory buffer
        Result<bool> load(uintptr_t data_ptr, size_t length);

        /// \brief Load model from JavaScript Uint8Array
        Result<bool> loadFromArray(const emscripten::val& uint8_array);

        /// \brief Get last error message
        std::string getLastError() const { return last_error_; }

        /// \brief Check if model loaded successfully
        bool isLoaded() const;

        /// \brief Get the model hierarchy
        const zenkit::ModelHierarchy& getHierarchy() const { return model_.hierarchy; }

        /// \brief Get the model mesh
        const zenkit::ModelMesh& getMesh() const { return model_.mesh; }

        /// \brief Get all soft-skin meshes (for animated characters)
        const std::vector<zenkit::SoftSkinMesh>& getSoftSkinMeshes() const { return model_.mesh.meshes; }

        /// \brief Get all attachment names as a vector (for JavaScript iteration)
        std::vector<std::string> getAttachmentNames() const;

        /// \brief Get all attachment meshes (for static geometry like chests)
        const std::unordered_map<std::string, zenkit::MultiResolutionMesh>& getAttachments() const { return model_.mesh.attachments; }

        /// \brief Get a specific attachment by name
        const zenkit::MultiResolutionMesh* getAttachment(const std::string& name) const;

        /// \brief Convert MultiResolutionMesh to ProcessedMeshData for Three.js rendering
        ProcessedMeshData convertAttachmentToProcessedMesh(const zenkit::MultiResolutionMesh* attachment) const;

        /// \brief Convert SoftSkinMesh to ProcessedMeshData for Three.js rendering
        ProcessedMeshData convertSoftSkinMeshToProcessedMesh(const zenkit::SoftSkinMesh* softSkinMesh) const;

        /// \brief Calculate required geometry offset relative to reference model
        /// \param softSkinMesh The soft-skin mesh to analyze
        /// \param hierarchy The model hierarchy with bone transforms
        /// \param referenceMesh Optional reference mesh (e.g., HUM_BODY_NAKED0) to calculate relative offset
        /// \return Vec3 offset that should be applied to geometry
        zenkit::Vec3 calculateGeometryOffset(const zenkit::SoftSkinMesh* softSkinMesh, const zenkit::ModelHierarchy* hierarchy, const zenkit::SoftSkinMesh* referenceMesh = nullptr) const;

        /// \brief Set hierarchy from a separately loaded ModelHierarchy
        void setHierarchy(const zenkit::ModelHierarchy& hierarchy) { model_.hierarchy = hierarchy; }

        /// \brief Set mesh from a separately loaded ModelMesh
        void setMesh(const zenkit::ModelMesh& mesh) { model_.mesh = mesh; }

    private:
        zenkit::Model model_;
        mutable std::string last_error_;
    };

    /// \brief Wrapper for ModelHierarchy that can be loaded separately
    class ModelHierarchyWrapper {
    public:
        ModelHierarchyWrapper() = default;
        ~ModelHierarchyWrapper() = default;

        ModelHierarchyWrapper(const ModelHierarchyWrapper&) = delete;
        ModelHierarchyWrapper& operator=(const ModelHierarchyWrapper&) = delete;
        ModelHierarchyWrapper(ModelHierarchyWrapper&&) = default;
        ModelHierarchyWrapper& operator=(ModelHierarchyWrapper&&) = default;

        /// \brief Load hierarchy from JavaScript Uint8Array
        Result<bool> loadFromArray(const emscripten::val& uint8_array);

        /// \brief Get last error message
        std::string getLastError() const { return last_error_; }

        /// \brief Get the hierarchy
        const zenkit::ModelHierarchy& getHierarchy() const { return hierarchy_; }

    private:
        zenkit::ModelHierarchy hierarchy_;
        mutable std::string last_error_;
    };

    /// \brief Wrapper for ModelMesh that can be loaded separately
    class ModelMeshWrapper {
    public:
        ModelMeshWrapper() = default;
        ~ModelMeshWrapper() = default;

        ModelMeshWrapper(const ModelMeshWrapper&) = delete;
        ModelMeshWrapper& operator=(const ModelMeshWrapper&) = delete;
        ModelMeshWrapper(ModelMeshWrapper&&) = default;
        ModelMeshWrapper& operator=(ModelMeshWrapper&&) = default;

        /// \brief Load mesh from JavaScript Uint8Array
        Result<bool> loadFromArray(const emscripten::val& uint8_array);

        /// \brief Get last error message
        std::string getLastError() const { return last_error_; }

        /// \brief Get the mesh
        const zenkit::ModelMesh& getMesh() const { return mesh_; }

    private:
        zenkit::ModelMesh mesh_;
        mutable std::string last_error_;
    };

    /// \brief Wrapper for MorphAnimation with WebAssembly-friendly interface
    class MorphAnimationWrapper {
    public:
        explicit MorphAnimationWrapper(const zenkit::MorphAnimation& anim) : anim_(anim) {}

        std::string getName() const { return anim_.name; }
        int32_t getLayer() const { return anim_.layer; }
        float getBlendIn() const { return anim_.blend_in; }
        float getBlendOut() const { return anim_.blend_out; }
        float getDuration() const { return anim_.duration; }
        float getSpeed() const { return anim_.speed; }
        uint8_t getFlags() const { return anim_.flags; }
        uint32_t getFrameCount() const { return anim_.frame_count; }
        const std::vector<uint32_t>& getVertices() const { return anim_.vertices; }
        const std::vector<zenkit::Vec3>& getSamples() const { return anim_.samples; }

    private:
        const zenkit::MorphAnimation& anim_;
    };

    /// \brief Wrapper for MorphMesh with WebAssembly-friendly interface
    class MorphMeshWrapper {
    public:
        MorphMeshWrapper() = default;
        explicit MorphMeshWrapper(const zenkit::MorphMesh& morph_mesh) : morph_mesh_(morph_mesh) {}
        ~MorphMeshWrapper() = default;

        // Non-copyable but movable
        MorphMeshWrapper(const MorphMeshWrapper&) = delete;
        MorphMeshWrapper& operator=(const MorphMeshWrapper&) = delete;
        MorphMeshWrapper(MorphMeshWrapper&&) = default;
        MorphMeshWrapper& operator=(MorphMeshWrapper&&) = default;

        /// \brief Load morph mesh from WebAssembly memory buffer
        Result<bool> load(uintptr_t data_ptr, size_t length);

        /// \brief Load morph mesh from JavaScript Uint8Array
        Result<bool> loadFromArray(const emscripten::val& uint8_array);

        /// \brief Get last error message
        std::string getLastError() const { return last_error_; }

        /// \brief Check if morph mesh loaded successfully
        bool isLoaded() const;

        /// \brief Get the underlying morph mesh
        const zenkit::MorphMesh& getMorphMesh() const { return morph_mesh_; }

        /// \brief Get the base mesh
        const zenkit::MultiResolutionMesh& getMesh() const { return morph_mesh_.mesh; }

        /// \brief Get animations count
        size_t getAnimationsCount() const { return morph_mesh_.animations.size(); }

        /// \brief Get morph positions count
        size_t getMorphPositionsCount() const { return morph_mesh_.morph_positions.size(); }

        /// \brief Convert MultiResolutionMesh to ProcessedMeshData for Three.js rendering
        ProcessedMeshData convertToProcessedMesh() const;

        /// \brief Get animation names
        std::vector<std::string> getAnimationNames() const;

    private:
        zenkit::MorphMesh morph_mesh_;
        mutable std::string last_error_;
    };

    /// \brief Wrapper for DaedalusScript to expose in WASM
    class DaedalusScriptWrapper {
    public:
        DaedalusScriptWrapper() = default;
        ~DaedalusScriptWrapper() = default;

        // Non-copyable, non-movable (DaedalusScript doesn't support move assignment)
        DaedalusScriptWrapper(const DaedalusScriptWrapper&) = delete;
        DaedalusScriptWrapper& operator=(const DaedalusScriptWrapper&) = delete;
        DaedalusScriptWrapper(DaedalusScriptWrapper&&) = default;
        DaedalusScriptWrapper& operator=(DaedalusScriptWrapper&&) = delete;

        /// \brief Load script from JavaScript Uint8Array
        Result<bool> loadFromArray(const emscripten::val& uint8_array);

        /// \brief Get last error message
        std::string getLastError() const { return last_error_; }

        /// \brief Check if script loaded successfully
        bool isLoaded() const { return script_.symbols().size() > 0; }

        /// \brief Get symbol count
        size_t getSymbolCount() const { return script_.symbols().size(); }

        /// \brief Get the underlying script
        zenkit::DaedalusScript& getScript() { return script_; }
        const zenkit::DaedalusScript& getScript() const { return script_; }

    private:
        zenkit::DaedalusScript script_;
        mutable std::string last_error_;
    };

    // Forward declaration
    struct ParamValue;
    
    /// \brief Wrapper for DaedalusVm to expose in WASM
    class DaedalusVmWrapper {
    public:
        /// \brief Create VM from script wrapper (takes ownership of script's internal script)
        explicit DaedalusVmWrapper(DaedalusScriptWrapper* script);

        ~DaedalusVmWrapper() = default;

        // Non-copyable, non-movable (DaedalusVm doesn't support move assignment)
        DaedalusVmWrapper(const DaedalusVmWrapper&) = delete;
        DaedalusVmWrapper& operator=(const DaedalusVmWrapper&) = delete;
        DaedalusVmWrapper(DaedalusVmWrapper&&) = default;
        DaedalusVmWrapper& operator=(DaedalusVmWrapper&&) = delete;

        /// \brief Get the underlying VM
        zenkit::DaedalusVm& getVm() { return vm_; }
        const zenkit::DaedalusVm& getVm() const { return vm_; }

        /// \brief Check if a symbol exists (without exposing raw pointer)
        bool hasSymbol(const std::string& name) const;

        /// \brief Get symbol count
        size_t getSymbolCount() const;

        /// \brief Get string value from a symbol (for instance members, pass instance symbol name)
        std::string getSymbolString(const std::string& symbolName, const std::string& instanceName = "");

        /// \brief Get int value from a symbol (for instance members, pass instance symbol name)
        int32_t getSymbolInt(const std::string& symbolName, const std::string& instanceName = "");

        /// \brief Get float value from a symbol (for instance members, pass instance symbol name)
        float getSymbolFloat(const std::string& symbolName, const std::string& instanceName = "");

        /// \brief Get symbol name from symbol index
        /// \param symbolIndex The symbol index
        /// \return Result containing the symbol name string, or error if index is invalid
        Result<std::string> getSymbolNameByIndex(int32_t symbolIndex);

        /// \brief Get symbol property value by instance index and property name
        /// \param instanceIndex The symbol index of the instance
        /// \param propertyName The name of the property to get
        /// \return Result containing the property value (as emscripten::val), or error
        /// 
        /// This is a convenience function that combines getSymbolNameByIndex with property access.
        /// It automatically determines the property type and returns the appropriate value.
        Result<emscripten::val> getInstancePropertyByIndex(int32_t instanceIndex, const std::string& propertyName);

        /// \brief Register a default external handler for unregistered external functions
        /// This prevents exceptions when script functions call unregistered externals
        void registerDefaultExternal();
        
        /// \brief Set a custom default external handler (called when unimplemented externals are invoked)
        /// \param callback JavaScript function that receives the external function name
        /// \return Result indicating success or error
        /// 
        /// When the VM calls an external function that hasn't been registered, this callback
        /// will be invoked with the function name. The VM automatically handles stack cleanup
        /// and return values, so the callback is just for logging/notification.
        Result<bool> setDefaultExternalHandler(const emscripten::val& callback);

        /// \brief Call a VM function with flexible parameters
        /// \param functionName The name of the function to call
        /// \param params JavaScript array of parameters (can be numbers, strings, or instance objects)
        /// \return Result containing the return value (as emscripten::val) or error
        /// 
        /// This method automatically:
        /// - Inspects the function signature to determine parameter types
        /// - Converts JavaScript values to appropriate C++ types (int, float, string, instance)
        /// - Handles void, int, float, string, and instance return types
        /// - Supports any number of parameters (up to 10)
        /// 
        /// Parameter types:
        /// - Numbers are converted to int or float based on function signature
        /// - Strings are passed as-is
        /// - Instance objects should have 'symbol_index' property or be instance name strings
        /// 
        /// Return value:
        /// - Void functions return undefined
        /// - Int/float functions return numbers
        /// - String functions return strings
        /// - Instance functions return objects with 'symbol_index' and 'name' properties
        Result<emscripten::val> callFunction(const std::string& functionName, const emscripten::val& params);

        /// \brief Register an external function with a JavaScript callback
        /// \param functionName The name of the external function to register
        /// \param callback JavaScript function to call when the external is invoked
        /// \return Result indicating success or error
        /// 
        /// The callback will receive parameters based on the function signature:
        /// - int parameters: passed as numbers
        /// - float parameters: passed as numbers
        /// - string parameters: passed as strings
        /// - instance parameters: passed as objects with symbol_index() method or instance name
        /// 
        /// For void functions, callback should return nothing.
        /// For functions with return values, callback should return the appropriate type.
        Result<bool> registerExternal(const std::string& functionName, const emscripten::val& callback);

        /// \brief Set the global 'self' variable (var C_NPC self)
        /// \param instanceName Instance name string or instance object with symbol_index
        /// \return Result indicating success or error
        /// 
        /// Many VM functions use the global 'self' variable to refer to the current NPC.
        /// This must be set before calling functions that use 'self'.
        Result<bool> setGlobalSelf(const emscripten::val& instanceName);

        /// \brief Set the global 'other' variable (var C_NPC other)
        /// \param instanceName Instance name string or instance object with symbol_index
        /// \return Result indicating success or error
        /// 
        /// Many VM functions use the global 'other' variable to refer to another NPC (usually the player).
        /// This must be set before calling functions that use 'other'.
        Result<bool> setGlobalOther(const emscripten::val& instanceName);

        /// \brief Set the global 'hero' variable (var C_NPC hero)
        /// \param instanceName Instance name string or instance object with symbol_index
        /// \return Result indicating success or error
        Result<bool> setGlobalHero(const emscripten::val& instanceName);

        /// \brief Set an arbitrary INSTANCE symbol to the given instance object
        /// \param symbolName Name of an INSTANCE symbol (e.g. "HERO")
        /// \param instanceName Instance name string or instance object with symbol_index
        /// \return Result indicating success or error
        Result<bool> setSymbolInstance(const std::string& symbolName, const emscripten::val& instanceName);

        /// \brief Initialize an instance by symbol index
        /// \param symbolIndex Symbol index of the instance to initialize
        /// \return Result containing the initialized instance or error message
        /// 
        /// This creates and initializes an instance if it doesn't exist.
        /// The instance definition code will be executed, setting all properties.
        Result<emscripten::val> initInstanceByIndex(int32_t symbolIndex);

    private:
        zenkit::DaedalusVm vm_;
        
        // Helper functions for symbol access
        // Get instance if it exists, return nullptr if not initialized
        // ZenKit user should ensure instances are initialized before accessing properties
        std::shared_ptr<zenkit::DaedalusInstance> getInstance(zenkit::DaedalusSymbol* instanceSym);
        zenkit::DaedalusSymbol* findMemberSymbol(zenkit::DaedalusSymbol* instanceSym, const std::string& symbolName);
        
        // Helper function to parse instance parameter from JavaScript
        Result<std::shared_ptr<zenkit::DaedalusInstance>> parseInstanceParameter(const emscripten::val& instanceName);
        
        // Storage for external function callbacks (universal registration)
        struct ExternalCallbackInfo {
            emscripten::val callback;
            std::vector<zenkit::DaedalusSymbol*> params;
            zenkit::DaedalusSymbol* sym;
        };
        std::unordered_map<std::string, ExternalCallbackInfo> externalCallbacks_;
        bool defaultExternalHandlerSet_ = false;
        
        // Universal external handler that routes to registered callbacks
        void handleUniversalExternal(zenkit::DaedalusVm& vm, zenkit::DaedalusSymbol& sym);
        
        // Helper to push JS return value to VM stack
        void pushJsReturnValue(zenkit::DaedalusVm& vm, const emscripten::val& result, zenkit::DaedalusDataType returnType);
        
    };

    /// \brief Wrapper for CutsceneLibrary to expose in WASM
    class CutsceneLibraryWrapper {
    public:
        CutsceneLibraryWrapper() = default;
        ~CutsceneLibraryWrapper() = default;

        // Non-copyable, non-movable
        CutsceneLibraryWrapper(const CutsceneLibraryWrapper&) = delete;
        CutsceneLibraryWrapper& operator=(const CutsceneLibraryWrapper&) = delete;
        CutsceneLibraryWrapper(CutsceneLibraryWrapper&&) = default;
        CutsceneLibraryWrapper& operator=(CutsceneLibraryWrapper&&) = delete;

        /// \brief Load cutscene library from JavaScript Uint8Array
        Result<bool> loadFromArray(const emscripten::val& uint8_array, int game_version = 1);

        /// \brief Get last error message
        std::string getLastError() const { return last_error_; }

        /// \brief Check if library loaded successfully
        bool isLoaded() const { return !library_.blocks.empty(); }

        /// \brief Get block count
        size_t getBlockCount() const { return library_.blocks.size(); }

        /// \brief Get block by name (output unit name)
        /// \param name The output unit name (e.g., "DIA_Xardas_FirstEXIT_15_00")
        /// \return Object with text and name properties, or null if not found
        emscripten::val getBlockByName(const std::string& name) const;

        /// \brief Get the underlying library
        zenkit::CutsceneLibrary& getLibrary() { return library_; }
        const zenkit::CutsceneLibrary& getLibrary() const { return library_; }

    private:
        zenkit::CutsceneLibrary library_;
        mutable std::string last_error_;
    };

    /// \brief Wrapper for ModelScript to expose in WASM
    class ModelScriptWrapper {
    public:
        ModelScriptWrapper() = default;
        ~ModelScriptWrapper() = default;

        ModelScriptWrapper(const ModelScriptWrapper&) = delete;
        ModelScriptWrapper& operator=(const ModelScriptWrapper&) = delete;
        ModelScriptWrapper(ModelScriptWrapper&&) = default;
        ModelScriptWrapper& operator=(ModelScriptWrapper&&) = default;

        /// \brief Load model script from JavaScript Uint8Array
        Result<bool> loadFromArray(const emscripten::val& uint8_array);

        /// \brief Get last error message
        std::string getLastError() const { return last_error_; }

        /// \brief Get skeleton name
        std::string getSkeletonName() const { return script_.skeleton.name; }

        /// \brief Check if skeleton mesh is disabled
        bool isSkeletonMeshDisabled() const { return script_.skeleton.disable_mesh; }

        /// \brief Get mesh count
        size_t getMeshCount() const { return script_.meshes.size(); }

        /// \brief Get mesh name at index
        std::string getMeshName(size_t index) const {
            if (index >= script_.meshes.size()) return "";
            return script_.meshes[index];
        }

        /// \brief Get disabled animation count
        size_t getDisabledAnimationCount() const { return script_.disabled_animations.size(); }

        /// \brief Get disabled animation name at index
        std::string getDisabledAnimationName(size_t index) const {
            if (index >= script_.disabled_animations.size()) return "";
            return script_.disabled_animations[index];
        }

        /// \brief Get animation count
        size_t getAnimationCount() const { return script_.animations.size(); }

        /// \brief Get animation name at index
        std::string getAnimationName(size_t index) const {
            if (index >= script_.animations.size()) return "";
            return script_.animations[index].name;
        }

        /// \brief Get animation layer at index
        uint32_t getAnimationLayer(size_t index) const {
            if (index >= script_.animations.size()) return 0;
            return script_.animations[index].layer;
        }

        /// \brief Get animation next name at index
        std::string getAnimationNext(size_t index) const {
            if (index >= script_.animations.size()) return "";
            return script_.animations[index].next;
        }

        /// \brief Get animation blend in at index
        float getAnimationBlendIn(size_t index) const {
            if (index >= script_.animations.size()) return 0.0f;
            return script_.animations[index].blend_in;
        }

        /// \brief Get animation blend out at index
        float getAnimationBlendOut(size_t index) const {
            if (index >= script_.animations.size()) return 0.0f;
            return script_.animations[index].blend_out;
        }

        /// \brief Get animation flags at index
        uint8_t getAnimationFlags(size_t index) const {
            if (index >= script_.animations.size()) return 0;
            return static_cast<uint8_t>(script_.animations[index].flags);
        }

        /// \brief Get animation model at index
        std::string getAnimationModel(size_t index) const {
            if (index >= script_.animations.size()) return "";
            return script_.animations[index].model;
        }

        /// \brief Get animation first frame at index
        int32_t getAnimationFirstFrame(size_t index) const {
            if (index >= script_.animations.size()) return 0;
            return script_.animations[index].first_frame;
        }

        /// \brief Get animation last frame at index
        int32_t getAnimationLastFrame(size_t index) const {
            if (index >= script_.animations.size()) return 0;
            return script_.animations[index].last_frame;
        }

        /// \brief Get animation FPS at index
        float getAnimationFps(size_t index) const {
            if (index >= script_.animations.size()) return 0.0f;
            return script_.animations[index].fps;
        }

        /// \brief Get animation speed at index
        float getAnimationSpeed(size_t index) const {
            if (index >= script_.animations.size()) return 0.0f;
            return script_.animations[index].speed;
        }

        /// \brief Get the underlying model script
        const zenkit::ModelScript& getScript() const { return script_; }

    private:
        zenkit::ModelScript script_;
        mutable std::string last_error_;
    };

    /// \brief Wrapper for ModelAnimation to expose in WASM
    class ModelAnimationWrapper {
    public:
        ModelAnimationWrapper() = default;
        ~ModelAnimationWrapper() = default;

        ModelAnimationWrapper(const ModelAnimationWrapper&) = delete;
        ModelAnimationWrapper& operator=(const ModelAnimationWrapper&) = delete;
        ModelAnimationWrapper(ModelAnimationWrapper&&) = default;
        ModelAnimationWrapper& operator=(ModelAnimationWrapper&&) = default;

        /// \brief Load model animation from JavaScript Uint8Array
        Result<bool> loadFromArray(const emscripten::val& uint8_array);

        /// \brief Get last error message
        std::string getLastError() const { return last_error_; }

        /// \brief Get animation name
        std::string getName() const { return animation_.name; }

        /// \brief Get next animation name
        std::string getNext() const { return animation_.next; }

        /// \brief Get layer
        uint32_t getLayer() const { return animation_.layer; }

        /// \brief Get frame count
        uint32_t getFrameCount() const { return animation_.frame_count; }

        /// \brief Get node count
        uint32_t getNodeCount() const { return animation_.node_count; }

        /// \brief Get FPS
        float getFps() const { return animation_.fps; }

        /// \brief Get source FPS
        float getFpsSource() const { return animation_.fps_source; }

        /// \brief Get sample count
        size_t getSampleCount() const { return animation_.samples.size(); }

        /// \brief Get sample at index (returns position and rotation)
        emscripten::val getSample(size_t frameIndex, size_t nodeIndex) const;

        /// \brief Get node index for a given node in the animation
        uint32_t getNodeIndex(size_t nodeIndex) const {
            if (nodeIndex >= animation_.node_indices.size()) return 0;
            return animation_.node_indices[nodeIndex];
        }

        /// \brief Get number of node indices (mapping MAN -> hierarchy)
        uint32_t getNodeIndexCount() const { return static_cast<uint32_t>(animation_.node_indices.size()); }

        /// \brief Get the underlying animation
        const zenkit::ModelAnimation& getAnimation() const { return animation_; }

    private:
        zenkit::ModelAnimation animation_;
        mutable std::string last_error_;
    };

    /// \brief Wrapper representing a single animated sample (position + rotation)
    struct AnimationSample {
        zenkit::Vec3 position;
        zenkit::Vec4 rotation;
    };

    /// \brief Wrapper for evaluating poses from a ModelAnimation over time
    ///
    /// This roughly corresponds to the Pose/AnimationSequence logic that was
    /// implemented in JavaScript in the MDS viewer.
    class PoseEvaluator {
    public:
        PoseEvaluator() = default;
        ~PoseEvaluator() = default;

        PoseEvaluator(const PoseEvaluator&) = delete;
        PoseEvaluator& operator=(const PoseEvaluator&) = delete;
        PoseEvaluator(PoseEvaluator&&) = default;
        PoseEvaluator& operator=(PoseEvaluator&&) = default;

        /// \brief Initialize evaluator from an animation
        ///
        /// Copies node indices and samples into a flattened representation.
        void setAnimation(const zenkit::ModelAnimation& animation);

        /// \brief Initialize evaluator from a ModelAnimationWrapper
        ///
        /// Convenience overload for WASM bindings – this lets JavaScript pass
        /// a ModelAnimation instance directly.
        void setAnimationFromWrapper(const ModelAnimationWrapper& wrapper);

        /// \brief Clear current animation data
        void clear();

        /// \brief Check if an animation is set
        bool hasAnimation() const noexcept { return frame_count_ > 0 && node_index_count_ > 0; }

        /// \brief Get total number of frames
        uint32_t getFrameCount() const noexcept { return frame_count_; }

        /// \brief Get node index count (mapping entries)
        uint32_t getNodeIndexCount() const noexcept { return node_index_count_; }

        /// \brief Get node index mapping at position
        uint32_t getNodeIndex(uint32_t index) const;

        /// \brief Get animation FPS
        float getFps() const noexcept { return fps_; }

        /// \brief Get total duration in milliseconds
        float getTotalTimeMs() const noexcept;

        /// \brief Evaluate pose at a given time (milliseconds)
        ///
        /// \param now_ms  Time since start in milliseconds
        /// \param loop    If true, time is wrapped into [0, totalTime)
        /// \return JavaScript array of AnimationSample-like objects
        emscripten::val evaluate(float now_ms, bool loop) const;

    private:
        uint32_t frame_count_ {0};
        uint32_t node_index_count_ {0};
        float fps_ {25.0f};
        std::vector<uint32_t> node_indices_;
        std::vector<AnimationSample> samples_;
    };

} // namespace zenkit::wasm
