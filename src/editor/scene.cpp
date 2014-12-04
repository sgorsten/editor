#include "scene.h"

#include <sstream>

void LightEnvironment::Bind(gl::Buffer & buffer, const gl::BlockDesc & perScene) const
{
    std::vector<GLubyte> dataBuffer(perScene.dataSize);    
    for(size_t i=0; i<lights.size(); ++i)
    {
        std::ostringstream ss; ss << "u_lights[" << i << "]"; auto obj = ss.str();
        perScene.SetUniform(dataBuffer.data(), obj+".position", lights[i].position);
        perScene.SetUniform(dataBuffer.data(), obj+".color", lights[i].color);
    }

    buffer.SetData(GL_UNIFORM_BUFFER, dataBuffer.size(), dataBuffer.data(), GL_STREAM_DRAW);
    buffer.BindBase(GL_UNIFORM_BUFFER, perScene.binding);
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

void Object::Draw()
{
    if(!prog || !mesh) return;

    auto model = ScaledTransformationMatrix(localScale, pose.orientation, pose.position);

    gl::Buffer buf;
    if(auto b = prog->GetNamedBlock("PerObject"))
    {
        std::vector<GLubyte> data(b->dataSize);
        b->SetUniform(data.data(), "u_model", model);
        b->SetUniform(data.data(), "u_modelIT", inv(transpose(model)));
        b->SetUniform(data.data(), "u_diffuse", color);
        if(light) b->SetUniform(data.data(), "u_emissive", light->color);
        buf.SetData(GL_UNIFORM_BUFFER, data.size(), data.data(), GL_STREAM_DRAW);
        buf.BindBase(GL_UNIFORM_BUFFER, b->binding);
    }
    
    prog->Use();
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

void Scene::Draw(RenderContext & ctx)
{
    LightEnvironment lights;
    for(auto & obj : objects) if(obj->light) lights.lights.push_back({obj->pose.position, obj->light->color});
    if(!objects.empty()) lights.Bind(ctx.perScene, *objects[0]->prog->GetNamedBlock("PerScene"));

    for(auto & obj : objects) obj->Draw();
}