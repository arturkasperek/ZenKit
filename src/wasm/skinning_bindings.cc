// Copyright © 2024 GothicKit Contributors.
// SPDX-License-Identifier: MIT
/// \file skinning_bindings.cc
/// \brief WebAssembly bindings for CPU skinning operations

#include "bindings_common.hh"

#include <emscripten/bind.h>
#include <cmath>

namespace zenkit::wasm {

    /// \brief Apply CPU skinning to vertices
    /// 
    /// This function performs matrix-based vertex skinning in WebAssembly for better performance.
    /// It processes all vertices efficiently using optimized loops.
    /// 
    /// \param boneMatrices Flat array of 4x4 matrices (boneCount * 16 floats, column-major)
    /// \param basePositions Base vertex positions (vertexCount * 3 floats)
    /// \param baseNormals Base vertex normals (vertexCount * 3 floats)
    /// \param vertexWeights Flat array: [vertex0_weightCount(uint32), boneIndex0(uint32), weight0(float), boneIndex1(uint32), weight1(float), ..., vertex1_weightCount, ...]
    ///                      Format: For each vertex: [weightCount(uint32), boneIndex0(uint32), weight0(float), boneIndex1(uint32), weight1(float), ...]
    ///                      Note: Weights are stored as floats but passed in a Uint32Array (bit pattern preserved)
    /// \param vertexCount Number of vertices
    /// \param boneCount Number of bones
    /// \return Object with 'positions' and 'normals' Float32Arrays
    emscripten::val applyCpuSkinning(
        const emscripten::val& boneMatricesVal,
        const emscripten::val& basePositionsVal,
        const emscripten::val& baseNormalsVal,
        const emscripten::val& vertexWeightsVal,
        uint32_t vertexCount,
        uint32_t boneCount
    ) {
        // Get typed array views from JavaScript
        // These are views into JS memory, we need to copy to WASM memory or work directly
        // For performance, we'll work with the data directly using typed_memory_view
        
        // Get array lengths
        const uint32_t boneMatrixCount = boneMatricesVal["length"].as<uint32_t>();
        const uint32_t basePosCount = basePositionsVal["length"].as<uint32_t>();
        const uint32_t baseNormCount = baseNormalsVal["length"].as<uint32_t>();
        const uint32_t weightArrayLength = vertexWeightsVal["length"].as<uint32_t>();
        
        // Copy data from JS arrays to C++ vectors for processing
        std::vector<float> boneMatricesVec(boneMatrixCount);
        std::vector<float> basePositionsVec(basePosCount);
        std::vector<float> baseNormalsVec(baseNormCount);
        std::vector<uint32_t> vertexWeightsVec(weightArrayLength);
        
        // Copy from JS typed arrays - element by element (could be optimized with memcpy)
        for (uint32_t i = 0; i < boneMatrixCount; ++i) {
            boneMatricesVec[i] = boneMatricesVal[i].as<float>();
        }
        for (uint32_t i = 0; i < basePosCount; ++i) {
            basePositionsVec[i] = basePositionsVal[i].as<float>();
        }
        for (uint32_t i = 0; i < baseNormCount; ++i) {
            baseNormalsVec[i] = baseNormalsVal[i].as<float>();
        }
        for (uint32_t i = 0; i < weightArrayLength; ++i) {
            vertexWeightsVec[i] = vertexWeightsVal[i].as<uint32_t>();
        }
        
        const float* boneMatrices = boneMatricesVec.data();
        const float* basePositions = basePositionsVec.data();
        const float* baseNormals = baseNormalsVec.data();
        const uint32_t* vertexWeights = vertexWeightsVec.data();
        
        // Allocate output arrays
        std::vector<float> positions(vertexCount * 3);
        std::vector<float> normals(vertexCount * 3);
        
        // Process each vertex
        size_t weightOffset = 0;
        for (uint32_t i = 0; i < vertexCount; ++i) {
            const uint32_t weightCount = vertexWeights[weightOffset++];
            
            if (weightCount == 0) {
                // No weights - use base position/normal
                const uint32_t idx = i * 3;
                positions[idx] = basePositions[idx];
                positions[idx + 1] = basePositions[idx + 1];
                positions[idx + 2] = basePositions[idx + 2];
                normals[idx] = baseNormals[idx];
                normals[idx + 1] = baseNormals[idx + 1];
                normals[idx + 2] = baseNormals[idx + 2];
                continue;
            }
            
            // Accumulate weighted transformations
            float resultPosX = 0.0f;
            float resultPosY = 0.0f;
            float resultPosZ = 0.0f;
            float resultNormX = 0.0f;
            float resultNormY = 0.0f;
            float resultNormZ = 0.0f;
            
            for (uint32_t w = 0; w < weightCount; ++w) {
                const uint32_t boneIndex = vertexWeights[weightOffset++];
                // Weight is stored as uint32_t but represents a float - need to reinterpret
                uint32_t weightBits = vertexWeights[weightOffset++];
                const float weight = *reinterpret_cast<const float*>(&weightBits);
                
                if (boneIndex >= boneCount) continue;
                
                // Get bone matrix (column-major, 16 floats per matrix)
                const float* m = &boneMatrices[boneIndex * 16];
                
                // Get base position
                const uint32_t posIdx = i * 3;
                const float px = basePositions[posIdx];
                const float py = basePositions[posIdx + 1];
                const float pz = basePositions[posIdx + 2];
                
                // Transform position: m * [px, py, pz, 1]
                const float tx = m[0] * px + m[4] * py + m[8] * pz + m[12];
                const float ty = m[1] * px + m[5] * py + m[9] * pz + m[13];
                const float tz = m[2] * px + m[6] * py + m[10] * pz + m[14];
                
                resultPosX += tx * weight;
                resultPosY += ty * weight;
                resultPosZ += tz * weight;
                
                // Transform normal (rotation part only, 3x3)
                const float nx = baseNormals[posIdx];
                const float ny = baseNormals[posIdx + 1];
                const float nz = baseNormals[posIdx + 2];
                
                const float ntx = m[0] * nx + m[4] * ny + m[8] * nz;
                const float nty = m[1] * nx + m[5] * ny + m[9] * nz;
                const float ntz = m[2] * nx + m[6] * ny + m[10] * nz;
                
                resultNormX += ntx * weight;
                resultNormY += nty * weight;
                resultNormZ += ntz * weight;
            }
            
            // Write results
            const uint32_t idx = i * 3;
            positions[idx] = resultPosX;
            positions[idx + 1] = resultPosY;
            positions[idx + 2] = resultPosZ;
            
            // Only write normals if they have length
            const float normLenSq = resultNormX * resultNormX + resultNormY * resultNormY + resultNormZ * resultNormZ;
            if (normLenSq > 0.0f) {
                normals[idx] = resultNormX;
                normals[idx + 1] = resultNormY;
                normals[idx + 2] = resultNormZ;
            } else {
                normals[idx] = baseNormals[idx];
                normals[idx + 1] = baseNormals[idx + 1];
                normals[idx + 2] = baseNormals[idx + 2];
            }
        }
        
        // Create JavaScript Float32Arrays
        emscripten::val Float32Array = emscripten::val::global("Float32Array");
        emscripten::val jsPositions = Float32Array.new_(emscripten::typed_memory_view(positions.size(), positions.data()));
        emscripten::val jsNormals = Float32Array.new_(emscripten::typed_memory_view(normals.size(), normals.data()));
        
        // Return result object
        emscripten::val result = emscripten::val::object();
        result.set("positions", jsPositions);
        result.set("normals", jsNormals);
        return result;
    }

} // namespace zenkit::wasm

EMSCRIPTEN_BINDINGS(zenkit_skinning) {
    using namespace zenkit::wasm;
    using namespace emscripten;
    
    function("applyCpuSkinning", &applyCpuSkinning);
}

