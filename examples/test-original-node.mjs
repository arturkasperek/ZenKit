import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';
import ZenKitModule from '../build-wasm/wasm/zenkit.mjs';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// Paths to assets
const base = '/Users/artur/dev/gothic/ZenKit';
const mdhPath = path.join(base, 'public/game-assets/ANIMS/_COMPILED/HUMANS.MDH');
const mdmPath = path.join(base, 'public/game-assets/ANIMS/_COMPILED/HUM_BODY_NAKED0.MDM');
const manPath = path.join(base, 'public/game-assets/ANIMS/_COMPILED/HUMANS-T_DIALOGGESTURE_01.MAN');

// Simple Mat4 helpers (column-major)
function matIdentity() {
  return [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1];
}
function matMul(a, b) {
  const r = new Array(16).fill(0);
  for (let c=0;c<4;++c){
    for (let rIdx=0;rIdx<4;++rIdx){
      r[c*4+rIdx] =
        a[0*4+rIdx]*b[c*4+0] +
        a[1*4+rIdx]*b[c*4+1] +
        a[2*4+rIdx]*b[c*4+2] +
        a[3*4+rIdx]*b[c*4+3];
    }
  }
  return r;
}
function matFromZen(mat) {
  const r = new Array(16);
  for (let c=0;c<4;++c) {
    r[c*4+0] = mat.get(c*4+0);
    r[c*4+1] = mat.get(c*4+1);
    r[c*4+2] = mat.get(c*4+2);
    r[c*4+3] = mat.get(c*4+3);
  }
  return r;
}
function matFromPosRot(pos, rot) {
  let {x,y,z,w} = rot;
  const n = Math.hypot(x,y,z,w);
  if (n>0) { x/=n; y/=n; z/=n; w/=n; }
  const r = matIdentity();
  r[0]=1-2*y*y-2*z*z; r[4]=2*x*y-2*z*w;   r[8]=2*x*z+2*y*w;
  r[1]=2*x*y+2*z*w;   r[5]=1-2*x*x-2*z*z; r[9]=2*y*z-2*x*w;
  r[2]=2*x*z-2*y*w;   r[6]=2*y*z+2*x*w;   r[10]=1-2*x*x-2*y*y;
  r[12]=pos.x; r[13]=pos.y; r[14]=pos.z; r[15]=1;
  return r;
}

function sampleAt(man, frame, nodeIndexMapped) {
  if (typeof man.getSample === 'function') {
    const s = man.getSample(frame, nodeIndexMapped);
    if (s && s.position) {
      return {
        position: { x: s.position.x, y: s.position.y, z: s.position.z },
        rotation: { x: s.rotation.x, y: s.rotation.y, z: s.rotation.z, w: s.rotation.w },
      };
    }
  }
  return { position:{x:0,y:0,z:0}, rotation:{x:0,y:0,z:0,w:1} };
}

async function main() {
  const ZenKit = await ZenKitModule();

  const mdhBuf = fs.readFileSync(mdhPath);
  const mdmBuf = fs.readFileSync(mdmPath);
  const manBuf = fs.readFileSync(manPath);

  const mdhLoader = ZenKit.createModelHierarchyLoader();
  mdhLoader.loadFromArray(new Uint8Array(mdhBuf));
  const mdh = mdhLoader.getHierarchy();

  const mdmLoader = ZenKit.createModelMeshLoader();
  mdmLoader.loadFromArray(new Uint8Array(mdmBuf));
  const model = ZenKit.createModel();
  model.setMesh(mdmLoader.getMesh());
  const softSkinMeshes = model.getSoftSkinMeshes();
  const ssm = softSkinMeshes.get(0);

  const man = ZenKit.createModelAnimation();
  man.loadFromArray(new Uint8Array(manBuf));

  const nodes = mdh.nodes;
  const nodeCount = nodes.size();
  const rootTr = mdh.rootTranslation;

  const frame = 30; // target frame for comparison
  // Frame animation (start from identity, apply samples)
  const animLocal = Array(nodeCount).fill(null).map(()=>matIdentity());
  const animWorld = Array(nodeCount).fill(null).map(()=>matIdentity());
  const animNodeCount = typeof man.getNodeCount === 'function' ? man.getNodeCount() : man.nodeCount;
  for (let j=0;j<animNodeCount;++j){
    const nodeId = typeof man.getNodeIndex === 'function' ? man.getNodeIndex(j) : j;
    if (nodeId >= nodeCount) continue;
    const s = sampleAt(man, frame, j);
    animLocal[nodeId] = matFromPosRot(s.position, s.rotation);
  }
  for (let i=0;i<nodeCount;++i){
    const n = nodes.get(i);
    const p = n.parentIndex;
    if (p >= 0) animWorld[i] = matMul(animWorld[p], animLocal[i]);
    else animWorld[i] = animLocal[i];
  }

  // Skinning
  const weights = ssm.weights;
  const positions = ssm.mesh.positions;
  const apply = (vidx) => {
    const ws = weights.get(vidx);
    let acc = { x:0, y:0, z:0 };
    for (let k=0;k<ws.size();++k){
      const w = ws.get(k);
      const m = animWorld[w.nodeIndex];
      const x = w.position.x, y=w.position.y, z=w.position.z;
      const tx = m[0]*x + m[4]*y + m[8]*z + m[12];
      const ty = m[1]*x + m[5]*y + m[9]*z + m[13];
      const tz = m[2]*x + m[6]*y + m[10]*z + m[14];
      acc.x += w.weight * tx;
      acc.y += w.weight * ty;
      acc.z += w.weight * tz;
    }
    return acc;
  };

  const logVertex = (idx) => {
    const sk = apply(idx);
    const b = positions.get(idx);
    console.log(`Vertex ${idx}: bind(${b.x.toFixed(4)},${b.y.toFixed(4)},${b.z.toFixed(4)}) skinned(${sk.x.toFixed(4)},${sk.y.toFixed(4)},${sk.z.toFixed(4)})`);
  };

  console.log(`Bones (frame ${frame}):`);
  [0,6,15,16,27,28].forEach(b=>{
    const m = animWorld[b];
    const n = nodes.get(b);
    console.log(`[${b}] ${n.name} pos(${m[12]},${m[13]},${m[14]})`);
  });
  console.log('Vertices:');
  logVertex(0); logVertex(50); logVertex(200);
}

main().catch(err => {
  console.error('Error:', err);
  process.exit(1);
});
