declare module 'zenkit' {
  export interface ZenKit {
    // Archive operations
    openVdf(path: string): Promise<VdfArchive>;
    createVdf(): VdfArchive;

    // World operations
    loadWorld(buffer: ArrayBuffer): Promise<World>;
    loadMesh(buffer: ArrayBuffer): Promise<Mesh>;

    // Texture operations
    loadTexture(buffer: ArrayBuffer): Promise<Texture>;
  }

  export interface VdfArchive {
    getEntry(name: string): VdfEntry | null;
    listEntries(): VdfEntry[];
    close(): void;
  }

  export interface VdfEntry {
    name: string;
    size: number;
    data: Uint8Array;
  }

  export interface World {
    // World data
  }

  export interface Mesh {
    // Mesh data
  }

  export interface Texture {
    // Texture data
  }

  const zenkit: ZenKit;
  export default zenkit;
}
