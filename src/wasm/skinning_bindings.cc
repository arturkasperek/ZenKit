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
    /// \param basePositions Base vertex positions (vertexCount * 3 floats) - used when vertex has no weights
    /// \param baseNormals Base vertex normals (vertexCount * 3 floats) - used when vertex has no weights
    /// \param vertexWeights Flat array: [vertex0_weightCount(uint32), boneIndex0(uint32), weight0(float as uint32), posX0(float as uint32), posY0(float as uint32), posZ0(float as uint32), hasNormal0(uint32), normX0(float as uint32), normY0(float as uint32), normZ0(float as uint32), ...]
    ///                      Format per weight: [boneIndex(uint32), weight(float as uint32), posX(float as uint32), posY(float as uint32), posZ(float as uint32), hasNormal(uint32), normX(float as uint32), normY(float as uint32), normZ(float as uint32)]
    ///                      Bone-local positions and normals are stored in the weight entries
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
        
        // Validate array sizes
        const uint32_t expectedPosCount = vertexCount * 3;
        const uint32_t expectedNormCount = vertexCount * 3;
        const uint32_t expectedBoneMatrixCount = boneCount * 16;
        
        if (basePosCount < expectedPosCount || baseNormCount < expectedNormCount) {
            // Return empty result if arrays are too small
            emscripten::val Float32Array = emscripten::val::global("Float32Array");
            emscripten::val result = emscripten::val::object();
            result.set("positions", Float32Array.new_(expectedPosCount));
            result.set("normals", Float32Array.new_(expectedNormCount));
            return result;
        }
        
        if (boneMatrixCount < expectedBoneMatrixCount) {
            // Return empty result if bone matrices are too small
            emscripten::val Float32Array = emscripten::val::global("Float32Array");
            emscripten::val result = emscripten::val::object();
            result.set("positions", Float32Array.new_(expectedPosCount));
            result.set("normals", Float32Array.new_(expectedNormCount));
            return result;
        }
        
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
        
        // Validate that we have enough data for all vertices
        if (basePositionsVec.size() < vertexCount * 3 || baseNormalsVec.size() < vertexCount * 3) {
            // Return empty result if arrays are too small
            emscripten::val Float32Array = emscripten::val::global("Float32Array");
            emscripten::val result = emscripten::val::object();
            result.set("positions", Float32Array.new_(vertexCount * 3));
            result.set("normals", Float32Array.new_(vertexCount * 3));
            return result;
        }
        
        const float* boneMatrices = boneMatricesVec.data();
        const float* basePositions = basePositionsVec.data();
        const float* baseNormals = baseNormalsVec.data();
        const uint32_t* vertexWeights = vertexWeightsVec.data();
        
        // Allocate output arrays
        std::vector<float> positions(vertexCount * 3);
        std::vector<float> normals(vertexCount * 3);
        
        // Process each vertex
        // Format: [weightCount, boneIndex, weight, posX, posY, posZ, hasNormal, normX, normY, normZ, ...]
        // Per weight: 9 uint32s (boneIndex, weight, posX, posY, posZ, hasNormal, normX, normY, normZ)
        size_t weightOffset = 0;
        const size_t weightArraySize = vertexWeightsVec.size();
        
        for (uint32_t i = 0; i < vertexCount; ++i) {
            const uint32_t idx = i * 3;
            
            // Check bounds before reading weight count
            if (weightOffset >= weightArraySize) {
                // Fallback to base position/normal if we run out of weight data
                positions[idx] = basePositions[idx];
                positions[idx + 1] = basePositions[idx + 1];
                positions[idx + 2] = basePositions[idx + 2];
                normals[idx] = baseNormals[idx];
                normals[idx + 1] = baseNormals[idx + 1];
                normals[idx + 2] = baseNormals[idx + 2];
                continue;
            }
            
            const uint32_t weightCount = vertexWeights[weightOffset++];
            
            if (weightCount == 0) {
                // No weights - use base position/normal
                positions[idx] = basePositions[idx];
                positions[idx + 1] = basePositions[idx + 1];
                positions[idx + 2] = basePositions[idx + 2];
                normals[idx] = baseNormals[idx];
                normals[idx + 1] = baseNormals[idx + 1];
                normals[idx + 2] = baseNormals[idx + 2];
                continue;
            }
            
            // Check if we have enough data for all weights (9 uint32s per weight)
            if (weightOffset + weightCount * 9 > weightArraySize) {
                // Not enough data - fallback to base position/normal
                positions[idx] = basePositions[idx];
                positions[idx + 1] = basePositions[idx + 1];
                positions[idx + 2] = basePositions[idx + 2];
                normals[idx] = baseNormals[idx];
                normals[idx + 1] = baseNormals[idx + 1];
                normals[idx + 2] = baseNormals[idx + 2];
                // Skip to end - remaining vertices will use base positions/normals
                weightOffset = weightArraySize;
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
                // Check bounds before reading weight data (9 uint32s per weight)
                if (weightOffset + 9 > weightArraySize) {
                    // Not enough data for this weight - skip remaining weights
                    break;
                }
                
                // Each weight entry is 9 uint32s: [boneIndex, weight, posX, posY, posZ, hasNormal, normX, normY, normZ]
                const uint32_t boneIndex = vertexWeights[weightOffset++];
                
                // Weight is stored as uint32_t but represents a float - need to reinterpret
                uint32_t weightBits = vertexWeights[weightOffset++];
                const float weight = *reinterpret_cast<const float*>(&weightBits);
                
                // Get bone-local position from weight entry (3 floats as uint32 bits)
                uint32_t posXBits = vertexWeights[weightOffset++];
                uint32_t posYBits = vertexWeights[weightOffset++];
                uint32_t posZBits = vertexWeights[weightOffset++];
                const float px = *reinterpret_cast<const float*>(&posXBits);
                const float py = *reinterpret_cast<const float*>(&posYBits);
                const float pz = *reinterpret_cast<const float*>(&posZBits);
                
                // Get normal flag
                const uint32_t hasNormal = vertexWeights[weightOffset++];
                
                // Get bone-local normal from weight entry (always 3 floats as uint32 bits, even if hasNormal is 0)
                uint32_t normXBits = vertexWeights[weightOffset++];
                uint32_t normYBits = vertexWeights[weightOffset++];
                uint32_t normZBits = vertexWeights[weightOffset++];
                
                // Validate bone index and matrix bounds
                if (boneIndex >= boneCount) continue;
                const size_t matrixOffset = static_cast<size_t>(boneIndex) * 16;
                if (matrixOffset + 15 >= boneMatricesVec.size()) continue;
                
                // Get bone matrix (column-major, 16 floats per matrix)
                // Safe to access m[0] through m[15] since we've validated bounds above
                const float* m = &boneMatrices[matrixOffset];
                
                // Transform bone-local position: m * [px, py, pz, 1]
                const float tx = m[0] * px + m[4] * py + m[8] * pz + m[12];
                const float ty = m[1] * px + m[5] * py + m[9] * pz + m[13];
                const float tz = m[2] * px + m[6] * py + m[10] * pz + m[14];
                
                resultPosX += tx * weight;
                resultPosY += ty * weight;
                resultPosZ += tz * weight;
                
                // Transform bone-local normal if present
                if (hasNormal) {
                    const float nx = *reinterpret_cast<const float*>(&normXBits);
                    const float ny = *reinterpret_cast<const float*>(&normYBits);
                    const float nz = *reinterpret_cast<const float*>(&normZBits);
                    
                    // Transform bone-local normal (rotation part only, 3x3)
                    const float ntx = m[0] * nx + m[4] * ny + m[8] * nz;
                    const float nty = m[1] * nx + m[5] * ny + m[9] * nz;
                    const float ntz = m[2] * nx + m[6] * ny + m[10] * nz;
                    
                    resultNormX += ntx * weight;
                    resultNormY += nty * weight;
                    resultNormZ += ntz * weight;
                }
            }
            
            // Write results (idx already defined above)
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
                // Fallback to base normal if transformed normal is invalid
                normals[idx] = baseNormals[idx];
                normals[idx + 1] = baseNormals[idx + 1];
                normals[idx + 2] = baseNormals[idx + 2];
            }
        }
        
        
        // Create JavaScript Float32Arrays - copy data to JS-owned memory
        // Pattern matches other bindings in bindings_common.hh
        emscripten::val Float32Array = emscripten::val::global("Float32Array");
        
        // Create JS arrays and copy data (this copies to JS-owned memory)
        emscripten::val jsPositions = Float32Array.new_(positions.size());
        emscripten::val jsNormals = Float32Array.new_(normals.size());
        
        // Copy data using set() method - wraps typed_memory_view in val to copy
        jsPositions.call<void>("set", emscripten::val(emscripten::typed_memory_view(positions.size(), positions.data())));
        jsNormals.call<void>("set", emscripten::val(emscripten::typed_memory_view(normals.size(), normals.data())));
        
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

