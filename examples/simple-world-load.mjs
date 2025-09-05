#!/usr/bin/env node

// ZenKit basic usage example for AI tools
import ZenKitModule from '../build-wasm/wasm/zenkit.mjs';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

async function basicZenKitExample() {
    // Initialize ZenKit
    const ZenKit = await ZenKitModule();
    
    // Load Gothic world file
    const zenPath = path.join(__dirname, '..', 'TOTENINSEL.ZEN');
    const fileData = fs.readFileSync(zenPath);
    const uint8Array = new Uint8Array(fileData);
    
    const world = ZenKit.createWorld();
    const result = world.loadFromArray(uint8Array); // Optional version: world.loadFromArray(uint8Array, 2)
    
    if (!result.success) {
        console.error('Load failed:', world.getLastError());
        return;
    }
    
    // Access world properties
    console.log('World loaded:', {
        npcSpawnEnabled: world.npcSpawnEnabled,
        hasPlayer: world.hasPlayer,
        hasSkyController: world.hasSkyController
    });
    
    // Access mesh data
    const mesh = world.mesh;
    console.log('Mesh stats:', {
        vertices: mesh.vertexCount,
        triangles: mesh.indexCount / 3,
        materials: mesh.materials.size()
    });
    
    // Get bounding box
    const bbox = {
        min: mesh.boundingBoxMin,
        max: mesh.boundingBoxMax
    };
    console.log('World bounds:', bbox);
    
    // Get WebGL-ready typed arrays
    const vertices = mesh.getVerticesTypedArray();    // Float32Array
    const normals = mesh.getNormalsTypedArray();      // Float32Array
    const uvs = mesh.getUVsTypedArray();              // Float32Array
    const indices = mesh.getIndicesTypedArray();      // Uint32Array
    
    console.log('WebGL arrays ready:', {
        vertices: vertices.length,
        normals: normals.length,
        uvs: uvs.length,
        indices: indices.length
    });
    
    // Access materials
    for (let i = 0; i < mesh.materials.size(); i++) {
        const material = mesh.materials.get(i);
        console.log(`Material ${i}:`, {
            name: material.name,
            texture: material.texture,
            group: material.group
        });
    }
    
    // Access individual vertices/features (with bounds checking)
    if (mesh.vertices.size() > 0) {
        const firstVertex = mesh.vertices.get(0);
        console.log('First vertex:', { x: firstVertex.x, y: firstVertex.y, z: firstVertex.z });
    }
    
    if (mesh.features.size() > 0) {
        const firstFeature = mesh.features.get(0);
        console.log('First feature:', {
            texture: { u: firstFeature.texture.x, v: firstFeature.texture.y },
            normal: { x: firstFeature.normal.x, y: firstFeature.normal.y, z: firstFeature.normal.z }
        });
    }
}

// Three.js integration example
function createThreeJSMesh(zenMesh) {
    // Basic Three.js BufferGeometry creation
    const geometry = new THREE.BufferGeometry();
    
    // Set attributes using ZenKit typed arrays (zero-copy!)
    geometry.setAttribute('position', new THREE.BufferAttribute(zenMesh.getVerticesTypedArray(), 3));
    geometry.setAttribute('normal', new THREE.BufferAttribute(zenMesh.getNormalsTypedArray(), 3));
    geometry.setAttribute('uv', new THREE.BufferAttribute(zenMesh.getUVsTypedArray(), 2));
    geometry.setIndex(new THREE.BufferAttribute(zenMesh.getIndicesTypedArray(), 1));
    
    // Set bounding box
    geometry.boundingBox = new THREE.Box3(
        new THREE.Vector3(zenMesh.boundingBoxMin.x, zenMesh.boundingBoxMin.y, zenMesh.boundingBoxMin.z),
        new THREE.Vector3(zenMesh.boundingBoxMax.x, zenMesh.boundingBoxMax.y, zenMesh.boundingBoxMax.z)
    );
    
    // Create mesh
    const material = new THREE.MeshLambertMaterial({ color: 0x888888 });
    const mesh = new THREE.Mesh(geometry, material);
    
    return mesh;
}

// Archive example
async function archiveExample() {
    const ZenKit = await ZenKitModule();
    const vdfData = fs.readFileSync('archive.vdf');
    const uint8Array = new Uint8Array(vdfData);
    
    const archive = ZenKit.createReadArchiveFromArray(uint8Array);
    
    // Read archive data (examples of different data types)
    archive.readString();     // Read string
    archive.readInt();        // Read integer
    archive.readFloat();      // Read float
    archive.readBool();       // Read boolean
    archive.readColor();      // Read color
    archive.readVec3();       // Read 3D vector
    archive.readVec2();       // Read 2D vector
}

basicZenKitExample().catch(console.error);