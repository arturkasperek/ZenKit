declare module '@kolarz3/zenkit' {
  export interface ZenKit {
    // World operations
    createWorld(): World;
    
    // Mesh operations
    createMesh(): Mesh;
    
    // Model operations
    createModel(): Model;
    
    // Model hierarchy loader (for loading .MDH files separately)
    createModelHierarchyLoader(): ModelHierarchyLoader;
    
    // Model mesh loader (for loading .MDM files separately)
    createModelMeshLoader(): ModelMeshLoader;
    
    // Morph mesh operations
    createMorphMesh(): MorphMesh;
    
    // Daedalus script operations
    createDaedalusScript(): DaedalusScript;
    
    // Daedalus VM operations (takes ownership of script)
    createDaedalusVm(script: DaedalusScript): DaedalusVm;
    
    // Cutscene library operations
    createCutsceneLibrary(): CutsceneLibrary;
    
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
    
    // Waypoint access
    getWaypointCount(): number;
    getWaypoint(index: number): WayPointResult;
    findWaypointByName(name: string): WayPointResult;
    getAllWaypoints(): WayPointData[];
    getWaypointEdgeCount(): number;
    getWaypointEdge(index: number): WayEdgeResult;
    
    // World mesh - directly exposes getProcessedMeshData() method
    mesh: {
      getProcessedMeshData(): ProcessedMeshData;
    };
  }

  export interface VobCollection {
    // Get number of VOBs
    size(): number;
    
    // Get VOB by index
    get(index: number): Vob;
  }

  export interface Vob {
    // VOB ID
    id: number;
    // Position in world space
    position: {
      x: number;
      y: number;
      z: number;
    };
    
    // Rotation matrix (3x3) - returns Emscripten TypedArrayLike object
    rotation: {
      toArray(): FloatArrayLike; // Returns Emscripten TypedArrayLike with 9 elements [m00, m01, m02, m10, m11, m12, m20, m21, m22]
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

  export interface WayPointData {
    name: string;
    position: Vector3;
    direction: Vector3;
    water_depth: number;
    under_water: boolean;
    free_point: boolean;
  }

  export interface WayEdgeData {
    waypoint_a_index: number;
    waypoint_b_index: number;
  }

  export interface WayPointResult {
    success: boolean;
    data: WayPointData;
    errorMessage?: string;
  }

  export interface WayEdgeResult {
    success: boolean;
    data: WayEdgeData;
    errorMessage?: string;
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
    vertices: FloatArrayLike;
    
    // Index data (indices into vertex array)
    indices: IntArrayLike;
    
    // Material data
    materials: MaterialArrayLike;
    
    // Material IDs per triangle
    materialIds: IntArrayLike;

    // Skinning weights (4 weights per vertex, packed)
    boneWeights?: FloatArrayLike;
    // Skinning indices (4 indices per vertex, packed)
    boneIndices?: IntArrayLike;
    // Bone-local positions (pos0..pos3) per vertex, packed 12 floats
    bonePositions?: FloatArrayLike;
  }

  export interface FloatArrayLike {
    size(): number;
    get(index: number): number;
  }

  export interface IntArrayLike {
    size(): number;
    get(index: number): number;
  }

  export interface MaterialArrayLike {
    size(): number;
    get(index: number): Material;
  }

  export interface StringArrayLike {
    size(): number;
    get(index: number): string;
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
    getAttachmentNames(): StringArrayLike; // Returns array of strings
    
    // Get attachment by name
    getAttachment(name: string): Attachment | null;
    
    // Get model hierarchy
    getHierarchy(): {
      nodes: {
        size?: () => number;
        get?: (index: number) => HierarchyNode;
        length?: number;
        [index: number]: HierarchyNode;
      };
      rootTranslation?: {
        x: number;
        y: number;
        z: number;
      };
    };
    
    // Set model hierarchy (for combining separately loaded hierarchy and mesh)
    setHierarchy(hierarchy: {
      nodes: {
        size?: () => number;
        get?: (index: number) => HierarchyNode;
        length?: number;
        [index: number]: HierarchyNode;
      };
    }): void;
    
    // Set model mesh (for combining separately loaded hierarchy and mesh)
    setMesh(mesh: {
      attachments: any; // Internal mesh structure
      meshes: any;
      checksum: number;
    }): void;
    
    // Convert attachment to processed mesh
    convertAttachmentToProcessedMesh(attachment: Attachment): ProcessedMeshData;
    
    // Get soft-skin meshes (for models without attachments)
    getSoftSkinMeshes(): {
      size(): number;
      get(index: number): SoftSkinMesh | null;
    };
    
    // Convert soft-skin mesh to processed mesh
    convertSoftSkinMeshToProcessedMesh(softSkinMesh: SoftSkinMesh): ProcessedMeshData;
  }
  
  export interface SoftSkinMesh {
    // Soft-skin mesh data (contains MultiResolutionMesh internally)
    mesh: any;
  }

  export interface ModelHierarchyLoader {
    // Load hierarchy from buffer (.MDH format)
    loadFromArray(buffer: Uint8Array): { success: boolean };
    
    // Get last error message
    getLastError(): string | null;
    
    // Get the loaded hierarchy
    getHierarchy(): {
      nodes: {
        size?: () => number;
        get?: (index: number) => HierarchyNode;
        length?: number;
        [index: number]: HierarchyNode;
      };
    };
  }

  export interface ModelMeshLoader {
    // Load mesh from buffer (.MDM format)
    loadFromArray(buffer: Uint8Array): { success: boolean };
    
    // Get last error message
    getLastError(): string | null;
    
    // Get the loaded mesh
    getMesh(): {
      attachments: any; // Internal mesh structure
      meshes: any;
      checksum: number;
    };
  }

  export interface Attachment {
    // Attachment data (mesh or other visual)
    // Implementation details depend on ZenKit internals
  }

  export interface HierarchyNode {
    name: string;
    parentIndex: number;
    getTransform(): {
      toArray(): number[]; // 16-element matrix array
    };
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
    getAnimationNames(): StringArrayLike; // Returns array of strings
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

  export interface DaedalusScript {
    // Load script from buffer (.DAT format)
    loadFromArray(buffer: Uint8Array): { success: boolean; error_message?: string };
    
    // Get last error message
    getLastError(): string;
    
    // Check if script is loaded
    isLoaded: boolean;
    
    // Get symbol count
    symbolCount: number;
  }

  export interface DaedalusVm {
    // Get symbol count
    symbolCount: number;
    
    // Check if a symbol exists
    hasSymbol(name: string): boolean;
    
    // Get string value from a symbol (for instance members, pass instance symbol name)
    getSymbolString(symbolName: string, instanceName?: string): string;
    
    // Get int value from a symbol (for instance members, pass instance symbol name)
    getSymbolInt(symbolName: string, instanceName?: string): number;
    
    // Get float value from a symbol (for instance members, pass instance symbol name)
    getSymbolFloat(symbolName: string, instanceName?: string): number;
    
    // Get symbol name from symbol index
    getSymbolNameByIndex(symbolIndex: number): StringResult;
    
    // Get instance property value by instance index and property name
    // Returns the property value (string, number, or instance object) based on property type
    getInstancePropertyByIndex(instanceIndex: number, propertyName: string): FunctionCallResult;
    
    // Call a VM function
    // Parameters: function name and array of parameters (numbers, strings, or instance objects)
    // Returns: Result with return value (number, string, instance object, or undefined for void)
    callFunction(functionName: string, params: any[]): FunctionCallResult;
    
    // Register an external function with a JavaScript callback
    // The callback will receive parameters based on the function signature
    // For void functions, callback should return nothing
    // For functions with return values, callback should return the appropriate type
    registerExternal(functionName: string, callback: (...args: any[]) => any): Result<boolean>;
    
    // Set the global 'self' variable (var C_NPC self)
    // Many VM functions use the global 'self' variable to refer to the current NPC
    setGlobalSelf(instanceName: string | DaedalusInstance): Result<boolean>;
    
    // Set the global 'other' variable (var C_NPC other)
    // Many VM functions use the global 'other' variable to refer to another NPC (usually the player)
    setGlobalOther(instanceName: string | DaedalusInstance): Result<boolean>;
    
    // Initialize an instance by symbol index
    // This creates and initializes an instance if it doesn't exist.
    // The instance definition code will be executed, setting all properties.
    initInstanceByIndex(symbolIndex: number): FunctionCallResult;
    
    // Set a default external handler callback for unregistered external functions
    // The callback receives the function name as a string
    setDefaultExternalHandler(callback: (functionName: string) => void): Result<boolean>;
  }
  
  export interface DaedalusInstance {
    // Symbol index of the instance
    symbol_index: number;
    
    // Name of the instance (if available)
    name?: string;
  }
  
  export interface FunctionCallResult {
    // Whether the function call succeeded
    success: boolean;
    
    // Return value (number, string, instance object, or undefined for void functions)
    data?: any;
    
    // Error message if the call failed
    errorMessage?: string;
  }
  
  export interface Result<T> {
    // Whether the operation succeeded
    success: boolean;
    
    // Result data (only present if success is true)
    data?: T;
    
    // Error message (only present if success is false)
    errorMessage?: string;
  }
  
  export interface CutsceneLibrary {
    // Load cutscene library from buffer (.BIN format)
    // gameVersion: 1 for Gothic 1, 2 for Gothic 2
    loadFromArray(buffer: Uint8Array, gameVersion?: number): Result<boolean>;
    
    // Get last error message
    getLastError(): string | null;
    
    // Check if library is loaded
    isLoaded: boolean;
    
    // Get number of blocks in the library
    blockCount: number;
    
    // Get a dialogue block by name
    // Returns an object with text, name, and blockName properties, or null if not found
    getBlockByName(name: string): CutsceneBlock | null;
  }
  
  export interface CutsceneBlock {
    // Dialogue text (Windows-1250 encoded, converted to UTF-8)
    text: string;
    
    // Message name (Windows-1250 encoded, converted to UTF-8)
    name: string;
    
    // Block name (Windows-1250 encoded, converted to UTF-8)
    blockName: string;
  }

  const zenkit: ZenKit;
  export default zenkit;
}
