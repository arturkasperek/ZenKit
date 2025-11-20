declare module 'zenkit' {
  export interface ZenKit {
    // World operations
    createWorld(): World;
    
    // Mesh operations
    createMesh(): Mesh;
    
    // Model operations
    createModel(): Model;
    
    // Morph mesh operations
    createMorphMesh(): MorphMesh;
    
    // Texture constructor
    Texture: new () => Texture;
  }

  export interface World {
    // Load world from buffer
    loadFromArray(buffer: Uint8Array): boolean;
    
    // Check if world is loaded
    isLoaded: boolean;
    
    // Get last error message
    getLastError(): string | null;
    
    // Get VOBs collection
    getVobs(): VobCollection;
    
    // World mesh
    mesh: Mesh;
  }

  export interface VobCollection {
    // Get number of VOBs
    size(): number;
    
    // Get VOB by index
    get(index: number): Vob;
  }

  export interface Vob {
    // Position in world space
    position: {
      x: number;
      y: number;
      z: number;
    };
    
    // Rotation matrix (3x3)
    rotation: {
      toArray(): number[]; // Returns 9-element array [m00, m01, m02, m10, m11, m12, m20, m21, m22]
    };
    
    // Visual properties
    visual: Visual;
    
    // Child VOBs
    children: VobCollection;
    
    // Whether to show visual
    showVisual: boolean;
    
    // VOB name (various possible property names)
    objectName?: string;
    name?: string;
    vobName?: string;
  }

  export interface Visual {
    // Visual type: 0=DECAL, 1=MESH, 2=MULTI_RES_MESH, 3=PARTICLE, 4=CAMERA, 5=MODEL, 6=MORPH_MESH
    type: number;
    
    // Visual name (mesh/model file path)
    name: string;
  }

  export interface Mesh {
    // Load mesh from buffer (.3DS format)
    loadFromArray(buffer: Uint8Array): { success: boolean };
    
    // Load multi-resolution mesh from buffer (.MRM format)
    loadMRMFromArray(buffer: Uint8Array): { success: boolean };
    
    // Get mesh data
    getMeshData(): MeshData;
  }

  export interface MeshData {
    // Get processed mesh data for rendering
    getProcessedMeshData(): ProcessedMeshData;
  }

  export interface ProcessedMeshData {
    // Vertex data (8 floats per vertex: x, y, z, nx, ny, nz, u, v)
    vertices: TypedArrayLike;
    
    // Index data (indices into vertex array)
    indices: TypedArrayLike;
    
    // Material data
    materials: TypedArrayLike;
    
    // Material IDs per triangle
    materialIds: TypedArrayLike;
  }

  export interface TypedArrayLike {
    // Get size/length
    size(): number;
    
    // Get value at index
    get(index: number): number | Material | string;
  }

  export interface Material {
    // Texture name/path
    texture: string;
  }

  export interface Model {
    // Load model from buffer (.MDL format)
    loadFromArray(buffer: Uint8Array): { success: boolean };
    
    // Check if model is loaded
    isLoaded: boolean;
    
    // Get last error message
    getLastError(): string | null;
    
    // Get attachment names
    getAttachmentNames(): TypedArrayLike; // Returns array of strings
    
    // Get attachment by name
    getAttachment(name: string): Attachment | null;
  }

  export interface Attachment {
    // Attachment data (mesh or other visual)
    // Implementation details depend on ZenKit internals
  }

  export interface MorphMesh {
    // Load morph mesh from buffer (.MMB format)
    loadFromArray(buffer: Uint8Array): { success: boolean };
    
    // Check if morph mesh is loaded
    isLoaded: boolean;
    
    // Get last error message
    getLastError(): string | null;
    
    // Convert to processed mesh for rendering
    convertToProcessedMesh(): ProcessedMeshData;
    
    // Get animation names
    getAnimationNames(): TypedArrayLike; // Returns array of strings
  }

  export interface Texture {
    // Load texture from buffer (.TEX format)
    loadFromArray(buffer: Uint8Array): { success: boolean } | null;
    
    // Texture width
    width: number;
    
    // Texture height
    height: number;
    
    // Get texture as RGBA8 array
    asRgba8(mipLevel: number): Uint8Array | null;
  }

  const zenkit: ZenKit;
  export default zenkit;
}
