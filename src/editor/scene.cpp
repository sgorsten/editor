#include "scene.h"

#include <sstream>

void LightEnvironment::Bind(GLuint program) const
{
    for(size_t i=0; i<lights.size(); ++i)
    {
        std::ostringstream ss; ss << "u_lights[" << i << "]"; auto obj = ss.str();
        gl::Uniform(program, obj+".position", lights[i].position);
        gl::Uniform(program, obj+".color", lights[i].color);
    }
    for(size_t i=lights.size(); i<8; ++i)
    {
        std::ostringstream ss; ss << "u_lights[" << i << "]"; auto obj = ss.str();
        gl::Uniform(program, obj+".position", float3(0,0,0));
        gl::Uniform(program, obj+".color", float3(0,0,0));
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
    auto model = ScaledTransformationMatrix(localScale, pose.orientation, pose.position);
    glUseProgram(prog);
    gl::Uniform(prog, "u_model", model);
    gl::Uniform(prog, "u_modelIT", inv(transpose(model)));
    gl::Uniform(prog, "u_modelViewProj", mul(viewProj, model));
    gl::Uniform(prog, "u_eye", eye);
    gl::Uniform(prog, "u_diffuse", color);
    gl::Uniform(prog, "u_emissive", lightColor);
    lights.Bind(prog);
    mesh->Draw();
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
    for(auto & obj : objects) if(mag2(obj->lightColor) > 0) lights.lights.push_back({obj->pose.position, obj->lightColor});
    for(auto & obj : objects) obj->Draw(viewProj, eye, lights);
}