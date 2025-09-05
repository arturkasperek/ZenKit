// Copyright © 2024 GothicKit Contributors.
// SPDX-License-Identifier: MIT
#include "bindings_common.hh"
#include "zenkit/World.hh"
#include "zenkit/Stream.hh"
#include "zenkit/Error.hh"
#include "zenkit/Mesh.hh"
#include "zenkit/world/BspTree.hh"

namespace zenkit::wasm {

    // MeshWrapper is now defined in bindings_common.hh

    /// \brief WebAssembly wrapper for zenkit::World that mirrors the C++ structure
    class WorldWrapper {
    public:
        WorldWrapper() = default;
        ~WorldWrapper() = default;

        // Non-copyable but movable
        WorldWrapper(const WorldWrapper&) = delete;
        WorldWrapper& operator=(const WorldWrapper&) = delete;
        WorldWrapper(WorldWrapper&&) = default;
        WorldWrapper& operator=(WorldWrapper&&) = default;

        /// \brief Load world from WebAssembly memory buffer
        Result<bool> load(uintptr_t data_ptr, size_t length) {
            try {
                auto reader = create_reader_from_buffer(data_ptr, length);
                world_.load(reader.get());
                last_error_.clear();
                return Result<bool>(true);
            } catch (const std::exception& e) {
                last_error_ = e.what();
                return Result<bool>(e.what());
            }
        }

        /// \brief Load world from JavaScript Uint8Array (automatic memory management)
        /// \param uint8_array JavaScript Uint8Array containing the world data
        /// \param version Optional Gothic game version (0 = auto-detect, 1 = Gothic 1, 2 = Gothic 2)
        Result<bool> loadFromArray(const emscripten::val& uint8_array, int version = 0) {
            try {
                auto reader = create_reader_from_js_array(uint8_array);
                
                if (version == 0) {
                    // Auto-detect version
                    world_.load(reader.get());
                } else {
                    // Use specific version
                    auto game_version = static_cast<GameVersion>(version);
                    world_.load(reader.get(), game_version);
                }
                
                last_error_.clear();
                return Result<bool>(true);
            } catch (const std::exception& e) {
                last_error_ = e.what();
                return Result<bool>(e.what());
            }
        }

        /// \brief Load world with specific game version
        Result<bool> loadWithVersion(uintptr_t data_ptr, size_t length, int version) {
            try {
                auto reader = create_reader_from_buffer(data_ptr, length);
                auto game_version = static_cast<GameVersion>(version);
                world_.load(reader.get(), game_version);
                last_error_.clear();
                return Result<bool>(true);
            } catch (const std::exception& e) {
                last_error_ = e.what();
                return Result<bool>(e.what());
            }
        }

        /// \brief Get last error message
        std::string getLastError() const {
            return last_error_;
        }

        /// \brief Check if world loaded successfully
        bool isLoaded() const {
            return !last_error_.empty() || world_.world_mesh.vertices.size() > 0;
        }

        // Direct access to World members (mirroring C++ structure)
        
        // Expose actual data as properties (not count functions)
        
        // Basic world info
        [[nodiscard]] bool getNpcSpawnEnabled() const { return world_.npc_spawn_enabled; }
        [[nodiscard]] int getNpcSpawnFlags() const { return world_.npc_spawn_flags; }
        [[nodiscard]] bool hasPlayer() const { return world_.player != nullptr; }
        [[nodiscard]] bool hasSkyController() const { return world_.sky_controller != nullptr; }

        // Access to underlying world object
        [[nodiscard]] const World& getWorld() const { return world_; }

        // Direct mesh access as property
        std::unique_ptr<MeshWrapper> getMesh() const {
            return std::make_unique<MeshWrapper>(world_.world_mesh);
        }

    private:
        World world_;
        std::string last_error_;
    };

    /// \brief Factory function for creating World instances
    std::unique_ptr<WorldWrapper> createWorld() {
        return std::make_unique<WorldWrapper>();
    }

} // namespace zenkit::wasm

// Emscripten bindings for World class
EMSCRIPTEN_BINDINGS(zenkit_world) {
    using namespace zenkit::wasm;
    using namespace emscripten;

    // Game version enum
    enum_<zenkit::GameVersion>("GameVersion")
        .value("GOTHIC_1", zenkit::GameVersion::GOTHIC_1)
        .value("GOTHIC_2", zenkit::GameVersion::GOTHIC_2);

    // Result template for bool operations
    class_<Result<bool>>("BoolResult")
        .property("success", &Result<bool>::success)
        .property("errorMessage", &Result<bool>::error_message);

    // Bind geometric structures
    value_object<Vector3>("Vector3")
        .field("x", &Vector3::x)
        .field("y", &Vector3::y)  
        .field("z", &Vector3::z);

    value_object<Vector2>("Vector2")
        .field("x", &Vector2::x)
        .field("y", &Vector2::y);

    value_object<VertexFeature>("VertexFeature")
        .field("texture", &VertexFeature::texture)
        .field("light", &VertexFeature::light)
        .field("normal", &VertexFeature::normal);

    value_object<MaterialData>("MaterialData")
        .field("name", &MaterialData::name)
        .field("group", &MaterialData::group)
        .field("texture", &MaterialData::texture);

    value_object<OrientedBoundingBoxData>("OrientedBoundingBoxData")
        .field("center", &OrientedBoundingBoxData::center)
        .field("axes", &OrientedBoundingBoxData::axes)
        .field("half_width", &OrientedBoundingBoxData::half_width);

    // Register vector types
    register_vector<Vector3>("VectorVector3");
    register_vector<Vector2>("VectorVector2");
    register_vector<VertexFeature>("VectorVertexFeature");
    register_vector<uint32_t>("VectorUint32");
    register_vector<MaterialData>("VectorMaterialData");

    // MeshData - expose actual data as properties with improved safety
    class_<MeshWrapper>("MeshData")
        .property("vertices", &MeshWrapper::getVertices)
        .property("features", &MeshWrapper::getFeatures)
        .property("vertexIndices", &MeshWrapper::getVertexIndices)
        .property("normals", &MeshWrapper::getNormals)
        .property("textureCoords", &MeshWrapper::getTextureCoords)
        .property("lightValues", &MeshWrapper::getLightValues)
        .property("materials", &MeshWrapper::getMaterials)
        .property("boundingBoxMin", &MeshWrapper::getBoundingBoxMin)
        .property("boundingBoxMax", &MeshWrapper::getBoundingBoxMax)
        .property("orientedBoundingBox", &MeshWrapper::getOrientedBoundingBox)
        .property("name", &MeshWrapper::getName)
        .property("vertexCount", &MeshWrapper::getVertexCount)
        .property("featureCount", &MeshWrapper::getFeatureCount)
        .property("indexCount", &MeshWrapper::getIndexCount)
        // Performance optimization methods for direct WebGL usage
        .function("getVerticesTypedArray", &MeshWrapper::getVerticesTypedArray)
        .function("getNormalsTypedArray", &MeshWrapper::getNormalsTypedArray)
        .function("getUVsTypedArray", &MeshWrapper::getUVsTypedArray)
        .function("getIndicesTypedArray", &MeshWrapper::getIndicesTypedArray);

    // Main World wrapper - properties instead of count functions
    class_<WorldWrapper>("World")
        // Loading methods (these are actions, so stay as functions)
        .function("load", &WorldWrapper::load)
        .function("loadFromArray", &WorldWrapper::loadFromArray)
        .function("loadWithVersion", &WorldWrapper::loadWithVersion)

        // Error handling methods
        .function("getLastError", &WorldWrapper::getLastError)
        .property("isLoaded", &WorldWrapper::isLoaded)

        // Properties (not count functions!)
        .property("npcSpawnEnabled", &WorldWrapper::getNpcSpawnEnabled)
        .property("npcSpawnFlags", &WorldWrapper::getNpcSpawnFlags)
        .property("hasPlayer", &WorldWrapper::hasPlayer)
        .property("hasSkyController", &WorldWrapper::hasSkyController)

        // Mesh access as property
        .property("mesh", &WorldWrapper::getMesh, allow_raw_pointers());

    // Factory function
    function("createWorld", &createWorld);
}
