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

    // Model script operations
    createModelScript(): ModelScript;

    // Model animation operations
    createModelAnimation(): ModelAnimation;

    // Pose evaluator operations
    createPoseEvaluator(): PoseEvaluator;

    // Texture constructor
    Texture: new () => Texture;
  }

  // Library information functions
  export function getZenKitVersion(): string;
  export function getLibraryInfo(): LibraryInfo;

  export interface World {
    // Load world from buffer
    loadFromArray(buffer: Uint8Array, version?: number): Result<boolean>;

    // Check if world is loaded
    isLoaded: boolean;

    // Get last error message
    getLastError(): string;

    // World properties
    npcSpawnEnabled: boolean;
    npcSpawnFlags: number;
    hasPlayer: boolean;
    hasSkyController: boolean;

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
    mesh: MeshData;
  }

  export interface VobData {
    id: number;
    vobName: string;
    type: number;
    position: Vector3;
    rotation: Matrix3x3Data;
    visual: Visual;
    showVisual: boolean;
    cdDynamic: boolean;
    children: VobData[];
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

  export interface Vector2 {
    x: number;
    y: number;
  }

  export interface Vector3 {
    x: number;
    y: number;
    z: number;
  }

  export interface OrientedBoundingBoxData {
    center: Vector3;
    axes: Vector3[];
    half_width: Vector3;
  }

  export interface Mesh {
    // Load mesh from buffer (.3DS format)
    loadFromArray(buffer: Uint8Array): Result<boolean>;

    // Load multi-resolution mesh from buffer (.MRM format)
    loadMRMFromArray(buffer: Uint8Array): Result<boolean>;

    // Get mesh data
    getMeshData(): MeshData;

    // Check if this is an MRM mesh
    isMRM(): boolean;
  }

  export interface MeshData {
    // Vertex data
    vertices: Vector3[];
    features: VertexFeature[];
    vertexIndices: number[];
    normals: Vector3[];
    textureCoords: Vector2[];
    lightValues: number[];
    materials: Material[];
    boundingBoxMin: Vector3;
    boundingBoxMax: Vector3;
    orientedBoundingBox: OrientedBoundingBoxData;
    name: string;
    vertexCount: number;
    featureCount: number;
    indexCount: number;

    // Performance optimization methods for direct WebGL usage
    getVerticesTypedArray(): Uint8Array | null;
    getNormalsTypedArray(): Float32Array | null;
    getUVsTypedArray(): Float32Array | null;
    getIndicesTypedArray(): Uint32Array | null;
    getFeatureIndicesTypedArray(): Uint32Array | null;
    getTriFeatureIndicesTypedArray(): Uint32Array | null;
    getPolygonMaterialIndicesTypedArray(): Uint32Array | null;

    // OpenGothic-style processed mesh data
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

  export interface VobCollection {
    size(): number;
    get(index: number): Vob;
  }

  export interface VertexFeature {
    textureCoords: Vector2;
    normal: Vector3;
    lightValue: number;
  }

  export interface StringResult extends Result<string> {}

  export interface Material {
    name?: string;
    group?: number;
    // Texture name/path
    texture: string;
    textureScale?: { x: number; y: number };
    smoothAngle?: number;
    /**
     * ZenGin: zCMaterial::noCollDet
     * If true, polygons with this material are skipped during collision detection (foliage, small branches, etc.).
     */
    disableCollision?: boolean;
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
    mesh: MultiResolutionMesh;
    bboxes: OrientedBoundingBoxData[];
    wedgeNormals: any[]; // SoftSkinWedgeNormal[]
    weights: any[][]; // VectorVectorSoftSkinWeightEntry
    nodes: number[];
  }

  export interface MultiResolutionMesh {
    positions: Vector3[];
    normals: Vector3[];
    subMeshes: SubMesh[];
    materials: Material[];
    bbox: BoundingBoxData;
    obbox: OrientedBoundingBoxData;
  }

  export interface SubMesh {
    mat: Material;
    triangles: any[]; // MeshTriangle[]
    wedges: any[]; // MeshWedge[]
    colors: number[];
    trianglePlaneIndices: number[];
    trianglePlanes: any[]; // MeshPlane[]
    wedgeMap: number[];
  }

  export interface BoundingBoxData {
    min: Vector3;
    max: Vector3;
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

    // Check if a symbol exists (without exposing raw pointer)
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
    getInstancePropertyByIndex(instanceIndex: number, propertyName: string): ValResult;

    // Call a VM function with flexible parameters
    callFunction(functionName: string, params: any[]): ValResult;

    // Register an external function with a JavaScript callback
    registerExternal(functionName: string, callback: (...args: any[]) => any): Result<boolean>;

    // Set the global 'self' variable (var C_NPC self)
    setGlobalSelf(instanceName: string | any): Result<boolean>;

    // Set the global 'other' variable (var C_NPC other)
    setGlobalOther(instanceName: string | any): Result<boolean>;

    // Initialize an instance by symbol index
    initInstanceByIndex(symbolIndex: number): ValResult;

    // Set a default external handler callback for unregistered external functions
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

  export interface ValResult extends Result<any> {}

  export interface Matrix3x3Data {
    get(row: number, col: number): number;
    getIndex(index: number): number;
    toArray(): number[];
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

  export interface LibraryInfo {
    version: string;
    buildType: string;
    hasMmap: boolean;
    debugBuild: boolean;
  }

  export interface ModelScript {
    // Load model script from buffer (.MSB format)
    loadFromArray(buffer: Uint8Array): Result<boolean>;

    // Get last error message
    getLastError(): string;

    // Get skeleton name
    getSkeletonName(): string;

    // Check if skeleton mesh is disabled
    isSkeletonMeshDisabled(): boolean;

    // Get mesh count
    getMeshCount(): number;

    // Get mesh name at index
    getMeshName(index: number): string;

    // Get disabled animation count
    getDisabledAnimationCount(): number;

    // Get disabled animation name at index
    getDisabledAnimationName(index: number): string;

    // Get animation count
    getAnimationCount(): number;

    // Get animation name at index
    getAnimationName(index: number): string;

    // Get animation layer at index
    getAnimationLayer(index: number): number;

    // Get animation next name at index
    getAnimationNext(index: number): string;

    // Get animation blend in at index
    getAnimationBlendIn(index: number): number;

    // Get animation blend out at index
    getAnimationBlendOut(index: number): number;

    // Get animation flags at index
    getAnimationFlags(index: number): number;

    // Get animation model at index
    getAnimationModel(index: number): string;

    // Get animation first frame at index
    getAnimationFirstFrame(index: number): number;

    // Get animation last frame at index
    getAnimationLastFrame(index: number): number;

    // Get animation FPS at index
    getAnimationFps(index: number): number;

    // Get animation speed at index
    getAnimationSpeed(index: number): number;
  }

  export interface ModelAnimation {
    // Load model animation from buffer (.MAN format)
    loadFromArray(buffer: Uint8Array): Result<boolean>;

    // Get last error message
    getLastError(): string;

    // Get animation name
    getName(): string;

    // Get next animation name
    getNext(): string;

    // Get layer
    getLayer(): number;

    // Get frame count
    getFrameCount(): number;

    // Get node count
    getNodeCount(): number;

    // Get node index count (mapping MAN -> hierarchy)
    getNodeIndexCount(): number;

    // Get FPS
    getFps(): number;

    // Get source FPS
    getFpsSource(): number;

    // Get sample count
    getSampleCount(): number;

    // Get sample at index (returns position and rotation)
    getSample(frameIndex: number, nodeIndex: number): AnimationSample | null;

    // Get node index mapping at position
    getNodeIndex(index: number): number;
  }

  export interface AnimationSample {
    position: Vector3;
    rotation: Vector4;
  }

  export interface Vector4 {
    x: number;
    y: number;
    z: number;
    w: number;
  }

  export interface PoseEvaluator {
    // Initialize evaluator from an animation
    setAnimation(animation: ModelAnimation): void;

    // Initialize evaluator from a ModelAnimationWrapper
    setAnimationFromWrapper(wrapper: ModelAnimation): void;

    // Clear current animation data
    clear(): void;

    // Check if an animation is set
    hasAnimation(): boolean;

    // Get total number of frames
    getFrameCount(): number;

    // Get node index count (mapping entries)
    getNodeIndexCount(): number;

    // Get node index mapping at position
    getNodeIndex(index: number): number;

    // Get animation FPS
    getFps(): number;

    // Get total duration in milliseconds
    getTotalTimeMs(): number;

    // Evaluate pose at a given time (milliseconds)
    evaluate(now_ms: number, loop: boolean): AnimationSample[];
  }

  const zenkit: ZenKit;
  export default zenkit;
}
