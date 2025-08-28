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

    // Vector and geometric wrapper classes to preserve C++ structure
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

    // MeshWrapper class - shared between world and mesh bindings
    class MeshWrapper {
    public:
        explicit MeshWrapper(const zenkit::Mesh& mesh) : mesh_(mesh) {}

        // Expose actual data as properties (not count functions)
        std::vector<Vector3> getVertices() const {
            std::vector<Vector3> positions;
            positions.reserve(mesh_.vertices.size());
            for (const auto& vertex : mesh_.vertices) {
                positions.emplace_back(vertex.x, vertex.y, vertex.z);
            }
            return positions;
        }

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

        // Individual feature components for convenience
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

        // Bounding box
        Vector3 getBoundingBoxMin() const {
            return Vector3(mesh_.bbox.min);
        }
        
        Vector3 getBoundingBoxMax() const {
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

    private:
        const zenkit::Mesh& mesh_;
    };

    // Forward declaration for World wrapper
    class WorldWrapper;

} // namespace zenkit::wasm
