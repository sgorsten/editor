#include "scene.h"

#include <sstream>

void LightEnvironment::Bind(const gl::Program & program) const
{
    for(size_t i=0; i<lights.size(); ++i)
    {
        std::ostringstream ss; ss << "u_lights[" << i << "]"; auto obj = ss.str();
        program.Uniform(obj+".position", lights[i].position);
        program.Uniform(obj+".color", lights[i].color);
    }
    for(size_t i=lights.size(); i<8; ++i)
    {
        std::ostringstream ss; ss << "u_lights[" << i << "]"; auto obj = ss.str();
        program.Uniform(obj+".position", float3(0,0,0));
        program.Uniform(obj+".color", float3(0,0,0));
    }
}

void Mesh::Upload()
{       
    glMesh.SetVertices(vertices);
    glMesh.SetAttribute(0, &Vertex::position);
    glMesh.SetAttribute(1, &Vertex::normal);
    glMesh.SetElements(triangles);
}

void Mesh::Draw() const
{
    glMesh.Draw();
}

void Object::Draw(const float4x4 & viewProj, const float3 & eye, const LightEnvironment & lights)
{
    if(!prog.IsValid() || !mesh.IsValid()) return;
    auto model = ScaledTransformationMatrix(localScale, pose.orientation, pose.position);
    glUseProgram(prog.GetAsset().GetObject());
    prog.GetAsset().Uniform("u_model", model);
    prog.GetAsset().Uniform("u_modelIT", inv(transpose(model)));
    prog.GetAsset().Uniform("u_modelViewProj", mul(viewProj, model));
    prog.GetAsset().Uniform("u_eye", eye);
    prog.GetAsset().Uniform("u_diffuse", color);
    prog.GetAsset().Uniform("u_emissive", float3(0,0,0));
    if(light)
    {
        prog.GetAsset().Uniform("u_emissive", light->color);
    }
    lights.Bind(prog.GetAsset());
    mesh.GetAsset().Draw();
}

std::shared_ptr<Object> Scene::Hit(const Ray & ray)
{
    std::shared_ptr<Object> best = nullptr;
    float bestT = 0;
    for(auto & obj : objects)
    {
        auto hit = obj->Hit(ray);
        if(hit.hit && (!best || hit.t < bestT))
        {
            best = obj;
            bestT = hit.t;
        }
    }
    return best;
}

void Scene::Draw(const float4x4 & viewProj, const float3 & eye)
{
    LightEnvironment lights;
    for(auto & obj : objects) if(obj->light) lights.lights.push_back({obj->pose.position, obj->light->color});
    for(auto & obj : objects) obj->Draw(viewProj, eye, lights);
}