# ZenKit + Three.js Integration Guide

Complete guide for integrating ZenKit WebAssembly with Three.js to build Gothic world editors and viewers.

---

## 🎯 **Overview**

This guide shows how to use the enhanced ZenKit WebAssembly library with Three.js to create Gothic world editors, viewers, and games. With the latest improvements, ZenKit provides automatic memory management, direct WebGL typed arrays, and seamless Three.js integration.

## 📋 **Prerequisites**

- Node.js 18+ or modern browser
- Three.js r150+
- Basic knowledge of JavaScript and Three.js
- Gothic .zen world files for testing

---

## 🚀 **Quick Start**

### **1. Installation**

```bash
# Install from npm (when published)
npm install @kolarz3/zenkit

# Or use the built WebAssembly files directly
# Copy zenkit.mjs, zenkit.wasm, zenkit.d.ts to your project
```

### **2. Basic Setup**

```javascript
import * as THREE from 'three';
import ZenKitModule from '@kolarz3/zenkit';

async function initGothicViewer() {
    // Initialize ZenKit WebAssembly
    const ZenKit = await ZenKitModule();
    console.log(`ZenKit Version: ${ZenKit.getZenKitVersion()}`);
    
    // Setup Three.js scene
    const scene = new THREE.Scene();
    const camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 1, 100000);
    const renderer = new THREE.WebGLRenderer({ antialias: true });
    renderer.setSize(window.innerWidth, window.innerHeight);
    document.body.appendChild(renderer.domElement);
    
    return { ZenKit, scene, camera, renderer };
}
```

---

## 📂 **Loading Gothic Worlds**

### **Method 1: From File (Node.js)**

```javascript
import fs from 'fs';

async function loadGothicWorld(ZenKit, filePath) {
    try {
        // Read file and create Uint8Array
        const fileBuffer = fs.readFileSync(filePath);
        const uint8Array = new Uint8Array(fileBuffer);
        
        // Load world with automatic memory management
        const world = ZenKit.createWorld();
        const result = world.loadFromArray(uint8Array);
        
        if (!result.success) {
            throw new Error(`Failed to load world: ${world.getLastError()}`);
        }
        
        console.log(`✅ World loaded: ${path.basename(filePath)}`);
        return world;
        
    } catch (error) {
        console.error('❌ World loading failed:', error.message);
        throw error;
    }
}

// Usage
const world = await loadGothicWorld(ZenKit, 'NEWWORLD.ZEN');
```

### **Method 2: From User Upload (Browser)**

```javascript
function setupFileUpload(ZenKit, onWorldLoaded) {
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = '.zen';
    
    input.addEventListener('change', async (event) => {
        const file = event.target.files[0];
        if (!file) return;
        
        try {
            // Read file as ArrayBuffer
            const arrayBuffer = await file.arrayBuffer();
            const uint8Array = new Uint8Array(arrayBuffer);
            
            // Load world with automatic memory management
            const world = ZenKit.createWorld();
            const result = world.loadFromArray(uint8Array);
            
            if (result.success) {
                onWorldLoaded(world, file.name);
            } else {
                console.error('Load failed:', world.getLastError());
            }
        } catch (error) {
            console.error('File reading failed:', error);
        }
    });
    
    return input;
}

// Usage
const fileInput = setupFileUpload(ZenKit, (world, filename) => {
    console.log(`Loaded ${filename}`);
    createThreeJSMesh(world.mesh, scene);
});
document.body.appendChild(fileInput);
```

### **Method 3: From URL (Fetch)**

```javascript
async function loadWorldFromURL(ZenKit, url) {
    try {
        console.log(`🌐 Fetching world from: ${url}`);
        const response = await fetch(url);
        
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        
        const arrayBuffer = await response.arrayBuffer();
        const uint8Array = new Uint8Array(arrayBuffer);
        
        const world = ZenKit.createWorld();
        const result = world.loadFromArray(uint8Array);
        
        if (!result.success) {
            throw new Error(`World loading failed: ${world.getLastError()}`);
        }
        
        return world;
        
    } catch (error) {
        console.error('❌ URL loading failed:', error.message);
        throw error;
    }
}

// Usage
const world = await loadWorldFromURL(ZenKit, '/assets/worlds/NEWWORLD.ZEN');
```

---

## 🎨 **Creating Three.js Geometry**

### **Basic Mesh Creation**

```javascript
function createThreeJSMesh(zenMesh, scene) {
    // Get WebGL-ready typed arrays (zero-copy!)
    const vertices = zenMesh.getVerticesTypedArray();    // Float32Array
    const normals = zenMesh.getNormalsTypedArray();      // Float32Array  
    const uvs = zenMesh.getUVsTypedArray();              // Float32Array
    const indices = zenMesh.getIndicesTypedArray();      // Uint32Array
    
    // Create Three.js BufferGeometry
    const geometry = new THREE.BufferGeometry();
    
    // Set attributes (direct typed array usage!)
    geometry.setAttribute('position', new THREE.BufferAttribute(vertices, 3));
    geometry.setAttribute('normal', new THREE.BufferAttribute(normals, 3));
    geometry.setAttribute('uv', new THREE.BufferAttribute(uvs, 2));
    geometry.setIndex(new THREE.BufferAttribute(indices, 1));
    
    // Set bounding box (no manual calculation needed!)
    geometry.boundingBox = new THREE.Box3(
        new THREE.Vector3(
            zenMesh.boundingBoxMin.x,
            zenMesh.boundingBoxMin.y,
            zenMesh.boundingBoxMin.z
        ),
        new THREE.Vector3(
            zenMesh.boundingBoxMax.x,
            zenMesh.boundingBoxMax.y,
            zenMesh.boundingBoxMax.z
        )
    );
    
    // Create material
    const material = new THREE.MeshLambertMaterial({
        color: 0x888888,
        wireframe: false
    });
    
    // Create mesh and add to scene
    const mesh = new THREE.Mesh(geometry, material);
    scene.add(mesh);
    
    console.log(`✅ Mesh created:`, {
        vertices: vertices.length / 3,
        triangles: indices.length / 3,
        materials: zenMesh.materials.size()
    });
    
    return mesh;
}
```

### **Advanced Mesh Creation with Materials**

```javascript
function createAdvancedThreeJSMesh(zenMesh, scene, textureLoader) {
    // Get geometry data
    const vertices = zenMesh.getVerticesTypedArray();
    const normals = zenMesh.getNormalsTypedArray();
    const uvs = zenMesh.getUVsTypedArray();
    const indices = zenMesh.getIndicesTypedArray();
    
    // Create geometry
    const geometry = new THREE.BufferGeometry();
    geometry.setAttribute('position', new THREE.BufferAttribute(vertices, 3));
    geometry.setAttribute('normal', new THREE.BufferAttribute(normals, 3));
    geometry.setAttribute('uv', new THREE.BufferAttribute(uvs, 2));
    geometry.setIndex(new THREE.BufferAttribute(indices, 1));
    
    // Set bounding box
    geometry.boundingBox = new THREE.Box3(
        new THREE.Vector3(zenMesh.boundingBoxMin.x, zenMesh.boundingBoxMin.y, zenMesh.boundingBoxMin.z),
        new THREE.Vector3(zenMesh.boundingBoxMax.x, zenMesh.boundingBoxMax.y, zenMesh.boundingBoxMax.z)
    );
    
    // Process materials
    const materials = [];
    const zenMaterials = zenMesh.materials;
    
    for (let i = 0; i < zenMaterials.size(); i++) {
        const zenMaterial = zenMaterials.get(i);
        
        // Create Three.js material
        const material = new THREE.MeshLambertMaterial({
            name: zenMaterial.name
        });
        
        // Load texture if available
        if (zenMaterial.texture && zenMaterial.texture.length > 0) {
            try {
                const texture = textureLoader.load(`/textures/${zenMaterial.texture}`);
                texture.wrapS = THREE.RepeatWrapping;
                texture.wrapT = THREE.RepeatWrapping;
                material.map = texture;
            } catch (error) {
                console.warn(`⚠️ Texture not found: ${zenMaterial.texture}`);
            }
        }
        
        materials.push(material);
    }
    
    // Create mesh with materials
    const material = materials.length > 1 ? materials : materials[0] || new THREE.MeshLambertMaterial();
    const mesh = new THREE.Mesh(geometry, material);
    
    scene.add(mesh);
    return mesh;
}

// Usage with texture loader
const textureLoader = new THREE.TextureLoader();
const mesh = createAdvancedThreeJSMesh(world.mesh, scene, textureLoader);
```

---


---

## 🎯 **Performance Optimization**

### **Instanced Rendering**

```javascript
function createInstancedMesh(zenMesh, positions, scene) {
    // Create base geometry
    const vertices = zenMesh.getVerticesTypedArray();
    const normals = zenMesh.getNormalsTypedArray();
    const uvs = zenMesh.getUVsTypedArray();
    const indices = zenMesh.getIndicesTypedArray();
    
    const geometry = new THREE.BufferGeometry();
    geometry.setAttribute('position', new THREE.BufferAttribute(vertices, 3));
    geometry.setAttribute('normal', new THREE.BufferAttribute(normals, 3));
    geometry.setAttribute('uv', new THREE.BufferAttribute(uvs, 2));
    geometry.setIndex(new THREE.BufferAttribute(indices, 1));
    
    // Create instanced mesh
    const material = new THREE.MeshLambertMaterial({ color: 0x888888 });
    const instancedMesh = new THREE.InstancedMesh(geometry, material, positions.length);
    
    // Set instance positions
    const matrix = new THREE.Matrix4();
    positions.forEach((position, i) => {
        matrix.setPosition(position.x, position.y, position.z);
        instancedMesh.setMatrixAt(i, matrix);
    });
    
    scene.add(instancedMesh);
    return instancedMesh;
}
```

---


---

## 📚 **Common Patterns**

### **Texture Management**

```javascript
class TextureManager {
    constructor() {
        this.loader = new THREE.TextureLoader();
        this.cache = new Map();
        this.basePath = '/textures/';
    }
    
    async loadTexture(textureName) {
        if (this.cache.has(textureName)) {
            return this.cache.get(textureName);
        }
        
        try {
            const texture = await this.loader.loadAsync(this.basePath + textureName);
            texture.wrapS = THREE.RepeatWrapping;
            texture.wrapT = THREE.RepeatWrapping;
            
            this.cache.set(textureName, texture);
            return texture;
            
        } catch (error) {
            console.warn(`⚠️ Texture not found: ${textureName}`);
            return null;
        }
    }
    
    createMaterial(zenMaterial) {
        const material = new THREE.MeshLambertMaterial({
            name: zenMaterial.name
        });
        
        if (zenMaterial.texture) {
            this.loadTexture(zenMaterial.texture).then(texture => {
                if (texture) {
                    material.map = texture;
                    material.needsUpdate = true;
                }
            });
        }
        
        return material;
    }
}
```


---

## 🎯 **Best Practices**

### **Memory Management**

```javascript
// ✅ DO: Use automatic memory management
const world = ZenKit.createWorld();
const result = world.loadFromArray(uint8Array);

// ❌ DON'T: Manual memory management (no longer needed)
// const wasmMemory = ZenKit._malloc(uint8Array.length);
// ZenKit._free(wasmMemory);
```

### **Error Handling**

```javascript
// ✅ DO: Check result and use getLastError()
const result = world.loadFromArray(uint8Array);
if (!result.success) {
    console.error('Load failed:', world.getLastError());
    return;
}

// ✅ DO: Validate mesh data
const mesh = world.mesh;
if (mesh.vertexCount === 0) {
    console.warn('Empty mesh loaded');
}
```

### **Performance**

```javascript
// ✅ DO: Use typed arrays directly
const vertices = mesh.getVerticesTypedArray();
geometry.setAttribute('position', new THREE.BufferAttribute(vertices, 3));

// ❌ DON'T: Convert to regular arrays
// const vertices = [];
// for (let i = 0; i < mesh.vertexCount; i++) {
//     const v = mesh.vertices.get(i);
//     vertices.push(v.x, v.y, v.z);
// }
```

### **Resource Cleanup**

```javascript
// ✅ DO: Dispose Three.js resources
function disposeMesh(mesh) {
    if (mesh.geometry) mesh.geometry.dispose();
    if (mesh.material) {
        if (Array.isArray(mesh.material)) {
            mesh.material.forEach(mat => mat.dispose());
        } else {
            mesh.material.dispose();
        }
    }
}
```

---

## 🚀 **Production Deployment**

### **Build Setup**

```javascript
// webpack.config.js
module.exports = {
    resolve: {
        fallback: {
            "fs": false,
            "path": false
        }
    },
    module: {
        rules: [
            {
                test: /\.wasm$/,
                type: 'webassembly/async'
            }
        ]
    },
    experiments: {
        asyncWebAssembly: true
    }
};
```

### **CDN Deployment**

```html
<!-- Include from CDN -->
<script type="module">
    import ZenKitModule from 'https://cdn.jsdelivr.net/npm/@kolarz3/zenkit@latest/zenkit.mjs';
    
    const ZenKit = await ZenKitModule();
    console.log('ZenKit loaded from CDN');
</script>
```

---

## 🎯 **Next Steps**

1. **Build an Editor**: Create tools for modifying Gothic worlds
2. **Add Physics**: Integrate with physics engines like Cannon.js or Ammo.js
3. **Create Tools**: Build importers/exporters for other formats

---

## 📖 **Additional Resources**

- [ZenKit Documentation](https://zk.gothickit.dev/)
- [Three.js Documentation](https://threejs.org/docs/)
- [WebAssembly Guide](https://developer.mozilla.org/en-US/docs/WebAssembly)
- [Gothic Modding Community](https://gothic-modding-community.github.io/)

---

**Happy Gothic world building! 🏰✨**
