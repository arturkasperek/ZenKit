#include <zenkit/ModelHierarchy.hh>
#include <zenkit/ModelMesh.hh>
#include <zenkit/ModelAnimation.hh>
#include <zenkit/Stream.hh>

#include <filesystem>
#include <iostream>
#include <vector>
#include <cmath>

namespace fs = std::filesystem;

struct Mat4f {
    float m[16]; // column-major
    static Mat4f identity() {
        Mat4f r{};
        r.m[0]=r.m[5]=r.m[10]=r.m[15]=1.f;
        return r;
    }
};

Mat4f mul(const Mat4f& a, const Mat4f& b) {
    Mat4f r{};
    for(int col=0; col<4; ++col){
        for(int row=0; row<4; ++row){
            r.m[col*4+row] =
                a.m[0*4+row]*b.m[col*4+0] +
                a.m[1*4+row]*b.m[col*4+1] +
                a.m[2*4+row]*b.m[col*4+2] +
                a.m[3*4+row]*b.m[col*4+3];
        }
    }
    return r;
}

Mat4f fromZen(const zenkit::Mat4& z) {
    Mat4f r{};
    for(int c=0;c<4;++c){
        r.m[c*4+0]=z.columns[c].x;
        r.m[c*4+1]=z.columns[c].y;
        r.m[c*4+2]=z.columns[c].z;
        r.m[c*4+3]=z.columns[c].w;
    }
    return r;
}

Mat4f fromPosRot(const zenkit::Vec3& p, const zenkit::Quat& q){
    // normalize
    float x=q.x, y=q.y, z=q.z, w=q.w;
    float n = std::sqrt(x*x+y*y+z*z+w*w);
    if(n>0){ x/=n; y/=n; z/=n; w/=n; }
    Mat4f r = Mat4f::identity();
    r.m[0]=1-2*y*y-2*z*z; r.m[4]=2*x*y-2*z*w;   r.m[8]=2*x*z+2*y*w;
    r.m[1]=2*x*y+2*z*w;   r.m[5]=1-2*x*x-2*z*z; r.m[9]=2*y*z-2*x*w;
    r.m[2]=2*x*z-2*y*w;   r.m[6]=2*y*z+2*x*w;   r.m[10]=1-2*x*x-2*y*y;
    r.m[12]=p.x; r.m[13]=p.y; r.m[14]=p.z; r.m[15]=1.f;
    return r;
}

zenkit::AnimationSample sampleAt(const zenkit::ModelAnimation& man, std::size_t frame, std::size_t nodeIdx) {
    const auto nc = man.node_count;
    const auto idx = frame*nc + nodeIdx;
    if(idx < man.samples.size()) return man.samples[idx];
    return zenkit::AnimationSample{};
}

int main() {
    try {
        fs::path base = "/Users/artur/dev/gothic/ZenKit";
        fs::path mdhPath = base / "public/game-assets/ANIMS/_COMPILED/HUMANS.MDH";
        fs::path mdmPath = base / "public/game-assets/ANIMS/_COMPILED/HUM_BODY_NAKED0.MDM";
        fs::path manPath = base / "public/game-assets/ANIMS/_COMPILED/HUMANS-T_DIALOGGESTURE_01.MAN";

        auto mdhStream = zenkit::Read::from(mdhPath.string());
        zenkit::ModelHierarchy mdh; mdh.load(mdhStream.get());

        auto mdmStream = zenkit::Read::from(mdmPath.string());
        zenkit::ModelMesh mdm; mdm.load(mdmStream.get());

        auto manStream = zenkit::Read::from(manPath.string());
        zenkit::ModelAnimation man; man.load(manStream.get());

        const auto& nodes = mdh.nodes;
        std::vector<Mat4f> bindLocal(nodes.size());
        std::vector<Mat4f> bindWorld(nodes.size());
        for(std::size_t i=0;i<nodes.size();++i){
            bindLocal[i]=fromZen(nodes[i].transform);
        }
        // apply rootTranslation to root node bind
        for(std::size_t i=0;i<nodes.size();++i){
            if(nodes[i].parent_index<0){
                bindLocal[i].m[12]+=mdh.root_translation.x;
                bindLocal[i].m[13]+=mdh.root_translation.y;
                bindLocal[i].m[14]+=mdh.root_translation.z;
            }
        }
        for(std::size_t i=0;i<nodes.size();++i){
            int p = nodes[i].parent_index;
            if(p>=0) bindWorld[i]=mul(bindWorld[p], bindLocal[i]);
            else bindWorld[i]=bindLocal[i];
        }

        const std::size_t frame = 30;
        // animation
        std::vector<Mat4f> animLocal(nodes.size(), Mat4f::identity());
        std::vector<Mat4f> animWorld(nodes.size(), Mat4f::identity());
        for(std::size_t j=0;j<man.node_indices.size();++j){
            std::size_t nodeId = man.node_indices[j];
            auto s = sampleAt(man, frame, j);
            animLocal[nodeId]=fromPosRot(s.position, s.rotation);
        }
        for(std::size_t i=0;i<nodes.size();++i){
            int p = nodes[i].parent_index;
            if(p>=0) animWorld[i]=mul(animWorld[p], animLocal[i]);
            else animWorld[i]=animLocal[i];
        }

        // skinning first soft-skin mesh
        const auto& ssm = mdm.meshes.at(0);
        const auto& weights = ssm.weights;
        const auto& positions = ssm.mesh.positions;

        auto apply = [&](std::size_t vidx)->zenkit::Vec3{
            zenkit::Vec3 acc{0,0,0};
            for(const auto& w: weights[vidx]){
                const auto& m = animWorld[w.node_index];
                const float x = w.position.x, y=w.position.y, z=w.position.z;
                float tx = m.m[0]*x + m.m[4]*y + m.m[8]*z + m.m[12];
                float ty = m.m[1]*x + m.m[5]*y + m.m[9]*z + m.m[13];
                float tz = m.m[2]*x + m.m[6]*y + m.m[10]*z + m.m[14];
                acc.x += w.weight * tx;
                acc.y += w.weight * ty;
                acc.z += w.weight * tz;
            }
            return acc;
        };

        auto logVertex = [&](int idx){
            auto sk = apply(idx);
            std::cout<<"Vertex "<<idx<<": bind("
                     <<positions[idx].x<<","<<positions[idx].y<<","<<positions[idx].z<<") skinned("
                     <<sk.x<<","<<sk.y<<","<<sk.z<<")\n";
        };

        std::cout<<"Bones (frame "<<frame<<"):\n";
        int roots[] = {0,6,15,16,27,28};
        for(int b: roots){
            const auto& m = animWorld[b];
            std::cout<<"["<<b<<"] "<<nodes[b].name<<" pos("
                     <<m.m[12]<<","<<m.m[13]<<","<<m.m[14]<<")\n";
        }
        std::cout<<"Vertices:\n";
        logVertex(0); logVertex(50); logVertex(200);
    } catch(const std::exception& e){
        std::cerr<<"Error: "<<e.what()<<"\n";
        return 1;
    }
    return 0;
}
