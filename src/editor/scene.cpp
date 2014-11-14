#include "scene.h"

#include <sstream>

void LightEnvironment::Bind(GLuint program) const
{
    for(size_t i=0; i<lights.size(); ++i)
    {
        std::ostringstream ss; ss << "u_lights[" << i << "].position";
        glUniform3fv(glGetUniformLocation(program, ss.str().c_str()), 1, &lights[i].position.x);

        ss.str({}); ss << "u_lights[" << i << "].color";
        glUniform3fv(glGetUniformLocation(program, ss.str().c_str()), 1, &lights[i].color.x);
    }
    for(size_t i=lights.size(); i<8; ++i)
    {
        std::ostringstream ss; ss << "u_lights[" << i << "].position";
        glUniform3f(glGetUniformLocation(program, ss.str().c_str()), 0, 0, 0);

        ss.str({}); ss << "u_lights[" << i << "].color";
        glUniform3f(glGetUniformLocation(program, ss.str().c_str()), 0, 0, 0);
    }
}

void Mesh::Upload()
{       
    glMesh.SetVertices(vertices);
    glMesh.SetAttribute(0, &Vertex::position);
    glMesh.SetAttribute(1, &Vertex::normal);
    glMesh.SetElements(triangles);
}

void Mesh::Draw()
{
    glMesh.Draw();
}

void Object::Draw(const float4x4 & viewProj, const float3 & eye, const LightEnvironment & lights)
{
    auto model = TranslationMatrix(position);
    auto mvp = mul(viewProj, model);
    glUseProgram(prog);
    glUniformMatrix4fv(glGetUniformLocation(prog, "u_model"), 1, GL_FALSE, &model.x.x);
    glUniformMatrix4fv(glGetUniformLocation(prog, "u_modelViewProj"), 1, GL_FALSE, &mvp.x.x);
    glUniform3fv(glGetUniformLocation(prog, "u_eye"), 1, &eye.x);
    glUniform3fv(glGetUniformLocation(prog, "u_diffuse"), 1, &color.x);
    glUniform3fv(glGetUniformLocation(prog, "u_emissive"), 1, &lightColor.x);
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
    for(auto & obj : objects) if(mag2(obj->lightColor) > 0) lights.lights.push_back({obj->position, obj->lightColor});
    for(auto & obj : objects) obj->Draw(viewProj, eye, lights);
}