// Copyright © 2024 GothicKit Contributors.
// SPDX-License-Identifier: MIT
#include "bindings_common.hh"
#include "zenkit/World.hh"
#include "zenkit/Stream.hh"
#include "zenkit/Error.hh"
#include "zenkit/Mesh.hh"
#include "zenkit/world/BspTree.hh"
#include "zenkit/world/WayNet.hh"

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

        // Get all VOBs from the world
        std::vector<VobData> getVobs() const {
            std::vector<VobData> vobs;
            vobs.reserve(world_.world_vobs.size());
            for (const auto& vob : world_.world_vobs) {
                if (vob) {
                    vobs.emplace_back(*vob);
                }
            }
            return vobs;
        }

        // Get waypoint count
        size_t getWaypointCount() const {
            return world_.world_way_net.waypoints.size();
        }

        // Get waypoint by index
        Result<WayPointData> getWaypoint(size_t index) const {
            if (index >= world_.world_way_net.waypoints.size()) {
                return Result<WayPointData>("Waypoint index out of range: " + std::to_string(index));
            }
            const auto& wp = world_.world_way_net.waypoints[index];
            WayPointData data;
            data.name = wp.name;
            data.position = Vector3{wp.position.x, wp.position.y, wp.position.z};
            data.direction = Vector3{wp.direction.x, wp.direction.y, wp.direction.z};
            data.water_depth = wp.water_depth;
            data.under_water = wp.under_water;
            data.free_point = wp.free_point;
            return Result<WayPointData>(std::move(data));
        }

        // Find waypoint by name
        Result<WayPointData> findWaypointByName(const std::string& name) const {
            for (const auto& wp : world_.world_way_net.waypoints) {
                if (wp.name == name) {
                    WayPointData data;
                    data.name = wp.name;
                    data.position = Vector3{wp.position.x, wp.position.y, wp.position.z};
                    data.direction = Vector3{wp.direction.x, wp.direction.y, wp.direction.z};
                    data.water_depth = wp.water_depth;
                    data.under_water = wp.under_water;
                    data.free_point = wp.free_point;
                    return Result<WayPointData>(std::move(data));
                }
            }
            return Result<WayPointData>("Waypoint not found: " + name);
        }

        // Get all waypoints
        std::vector<WayPointData> getAllWaypoints() const {
            std::vector<WayPointData> result;
            result.reserve(world_.world_way_net.waypoints.size());
            for (const auto& wp : world_.world_way_net.waypoints) {
                WayPointData data;
                data.name = wp.name;
                data.position = Vector3{wp.position.x, wp.position.y, wp.position.z};
                data.direction = Vector3{wp.direction.x, wp.direction.y, wp.direction.z};
                data.water_depth = wp.water_depth;
                data.under_water = wp.under_water;
                data.free_point = wp.free_point;
                result.push_back(std::move(data));
            }
            return result;
        }

        // Get waypoint edge count
        size_t getWaypointEdgeCount() const {
            return world_.world_way_net.edges.size();
        }

        // Get waypoint edge by index
        Result<WayEdgeData> getWaypointEdge(size_t index) const {
            if (index >= world_.world_way_net.edges.size()) {
                return Result<WayEdgeData>("Waypoint edge index out of range: " + std::to_string(index));
            }
            const auto& edge = world_.world_way_net.edges[index];
            WayEdgeData data;
            data.waypoint_a_index = edge.a;
            data.waypoint_b_index = edge.b;
            return Result<WayEdgeData>(std::move(data));
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

    // Waypoint data structures
    value_object<WayPointData>("WayPointData")
        .field("name", &WayPointData::name)
        .field("position", &WayPointData::position)
        .field("direction", &WayPointData::direction)
        .field("water_depth", &WayPointData::water_depth)
        .field("under_water", &WayPointData::under_water)
        .field("free_point", &WayPointData::free_point);

    value_object<WayEdgeData>("WayEdgeData")
        .field("waypoint_a_index", &WayEdgeData::waypoint_a_index)
        .field("waypoint_b_index", &WayEdgeData::waypoint_b_index);

    // Result types for waypoint operations
    class_<Result<WayPointData>>("WayPointResult")
        .property("success", &Result<WayPointData>::success)
        .property("data", &Result<WayPointData>::data)
        .property("errorMessage", &Result<WayPointData>::error_message);

    class_<Result<WayEdgeData>>("WayEdgeResult")
        .property("success", &Result<WayEdgeData>::success)
        .property("data", &Result<WayEdgeData>::data)
        .property("errorMessage", &Result<WayEdgeData>::error_message);

    // Model mesh structures
    value_object<zenkit::SubMesh>("SubMesh")
        .field("mat", &zenkit::SubMesh::mat)
        .field("triangles", &zenkit::SubMesh::triangles)
        .field("wedges", &zenkit::SubMesh::wedges)
        .field("colors", &zenkit::SubMesh::colors)
        .field("trianglePlaneIndices", &zenkit::SubMesh::triangle_plane_indices)
        .field("trianglePlanes", &zenkit::SubMesh::triangle_planes)
        .field("wedgeMap", &zenkit::SubMesh::wedge_map);

    value_object<zenkit::SoftSkinWedgeNormal>("SoftSkinWedgeNormal")
        .field("normal", &zenkit::SoftSkinWedgeNormal::normal)
        .field("index", &zenkit::SoftSkinWedgeNormal::index);

    value_object<zenkit::SoftSkinWeightEntry>("SoftSkinWeightEntry")
        .field("weight", &zenkit::SoftSkinWeightEntry::weight)
        .field("position", &zenkit::SoftSkinWeightEntry::position)
        .field("nodeIndex", &zenkit::SoftSkinWeightEntry::node_index);

    // Matrix3x3Data is registered in zenkit_wasm.cc

    // VisualData for VOB visuals
    value_object<VisualData>("VisualData")
        .field("name", &VisualData::name)
        .field("type", &VisualData::type);

    // VobData - Visual Objects
    value_object<VobData>("VobData")
        .field("id", &VobData::id)
        .field("vobName", &VobData::vob_name)
        .field("type", &VobData::type)
        .field("position", &VobData::position)
        .field("rotation", &VobData::rotation)
        .field("visual", &VobData::visual)
        .field("showVisual", &VobData::show_visual)
        .field("cdDynamic", &VobData::cd_dynamic)
        .field("children", &VobData::children);

    // ProcessedMeshData - OpenGothic-style processed mesh
    value_object<ProcessedMeshData>("ProcessedMeshData")
        .field("vertices", &ProcessedMeshData::vertices)
        .field("indices", &ProcessedMeshData::indices)
        .field("materialIds", &ProcessedMeshData::materialIds)
        .field("materials", &ProcessedMeshData::materials);

    // Register vector types
    register_vector<Vector3>("VectorVector3");
    register_vector<Vector2>("VectorVector2");
    register_vector<VertexFeature>("VectorVertexFeature");
    register_vector<uint32_t>("VectorUint32");
    register_vector<float>("VectorFloat");
    register_vector<MaterialData>("VectorMaterialData");
    register_vector<VobData>("VectorVobData");
    register_vector<WayPointData>("VectorWayPointData");

    // Model mesh vector types
    register_vector<zenkit::SubMesh>("VectorSubMesh");
    register_vector<zenkit::SoftSkinMesh>("VectorSoftSkinMesh");
    register_vector<zenkit::SoftSkinWedgeNormal>("VectorSoftSkinWedgeNormal");
    register_vector<zenkit::SoftSkinWeightEntry>("VectorSoftSkinWeightEntry");
    register_vector<zenkit::OrientedBoundingBox>("VectorOrientedBoundingBox");
    register_vector<int32_t>("VectorInt32");
    register_vector<std::string>("VectorString");

    // Register MultiResolutionMesh as a value type first
    value_object<zenkit::MultiResolutionMesh>("MultiResolutionMeshValue")
        .field("positions", &zenkit::MultiResolutionMesh::positions)
        .field("normals", &zenkit::MultiResolutionMesh::normals)
        .field("subMeshes", &zenkit::MultiResolutionMesh::sub_meshes)
        .field("materials", &zenkit::MultiResolutionMesh::materials)
        .field("bbox", &zenkit::MultiResolutionMesh::bbox)
        .field("obbox", &zenkit::MultiResolutionMesh::obbox);

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
        .function("getIndicesTypedArray", &MeshWrapper::getIndicesTypedArray)
        .function("getFeatureIndicesTypedArray", &MeshWrapper::getFeatureIndicesTypedArray)
        .function("getTriFeatureIndicesTypedArray", &MeshWrapper::getTriFeatureIndicesTypedArray)
        .function("getPolygonMaterialIndicesTypedArray", &MeshWrapper::getPolygonMaterialIndicesTypedArray)
        // OpenGothic-style processed mesh data
        .function("getProcessedMeshData", &MeshWrapper::getProcessedMeshData);

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
        .property("mesh", &WorldWrapper::getMesh, allow_raw_pointers())
        
        // VOBs access
        .function("getVobs", &WorldWrapper::getVobs)
        
        // Waypoint access
        .function("getWaypointCount", &WorldWrapper::getWaypointCount)
        .function("getWaypoint", &WorldWrapper::getWaypoint)
        .function("findWaypointByName", &WorldWrapper::findWaypointByName)
        .function("getAllWaypoints", &WorldWrapper::getAllWaypoints)
        .function("getWaypointEdgeCount", &WorldWrapper::getWaypointEdgeCount)
        .function("getWaypointEdge", &WorldWrapper::getWaypointEdge);

    // Standalone Mesh class for loading mesh files
    class_<StandaloneMeshWrapper>("Mesh")
        .function("loadFromArray", &StandaloneMeshWrapper::loadFromArray)
        .function("loadMRMFromArray", &StandaloneMeshWrapper::loadMRMFromArray)
        .function("getMeshData", &StandaloneMeshWrapper::getMeshData, allow_raw_pointers())
        .function("isMRM", &StandaloneMeshWrapper::isMRM);

    // MultiResolutionMesh wrapper for model attachments
    class_<zenkit::MultiResolutionMesh>("MultiResolutionMesh")
        .property("positions", &zenkit::MultiResolutionMesh::positions)
        .property("normals", &zenkit::MultiResolutionMesh::normals)
        .property("subMeshes", &zenkit::MultiResolutionMesh::sub_meshes)
        .property("materials", &zenkit::MultiResolutionMesh::materials)
        .property("bbox", &zenkit::MultiResolutionMesh::bbox)
        .property("obbox", &zenkit::MultiResolutionMesh::obbox);

    // SoftSkinMesh wrapper for animated models
    class_<zenkit::SoftSkinMesh>("SoftSkinMesh")
        .property("mesh", &zenkit::SoftSkinMesh::mesh)
        .property("bboxes", &zenkit::SoftSkinMesh::bboxes)
        .property("wedgeNormals", &zenkit::SoftSkinMesh::wedge_normals)
        .property("weights", &zenkit::SoftSkinMesh::weights)
        .property("nodes", &zenkit::SoftSkinMesh::nodes);

    // Factory functions
    function("createWorld", &createWorld);
    function("createMesh", select_overload<std::unique_ptr<StandaloneMeshWrapper>()>([]() {
        return std::make_unique<StandaloneMeshWrapper>();
    }));
}
