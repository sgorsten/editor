#include "load.h"

#include <map>
#include <fstream>
#include <sstream>

Mesh LoadMeshFromObj(const std::string & filepath)
{
    std::vector<Vertex> vertices;
    std::map<std::string, size_t> indices;
    std::vector<uint3> triangles;

    std::vector<float4> positions;
    std::vector<float3> texCoords, normals;

    std::string line;
    std::ifstream in(filepath);
    while(in)
    {
        std::getline(in, line);
        if(!in) break;
        
        std::istringstream inLine(line);
        std::string token;
        inLine >> token;
        if(token == "v")
        {
            float4 position = {0,0,0,1};
            inLine >> position.x >> position.y >> position.z >> position.w;
            positions.push_back(position);
        }
        if(token == "vt")
        {
            float3 texCoord = {0,0,0};
            inLine >> texCoord.x >> texCoord.y >> texCoord.z;
            texCoords.push_back(texCoord);
        }
        if(token == "vn")
        {
            float3 normal = {0,0,0};
            inLine >> normal.x >> normal.y >> normal.z;
            normals.push_back(norm(normal));
        }

        if(token == "f")
        {
            std::vector<size_t> faceIndices;
            while(inLine)
            {
                inLine >> token;
                if(!inLine) break;
                auto it = indices.find(token);
                if(it != end(indices))
                {
                    faceIndices.push_back(it->second);
                    continue;
                }
                indices[token] = vertices.size();
                faceIndices.push_back(vertices.size());

                Vertex vertex;
                bool skipTexCoords = token.find("//") != std::string::npos;
                for(auto & ch : token) if(ch == '/') ch = ' ';
                int i0=0, i1=0, i2=0;
                std::istringstream(token) >> i0 >> i1 >> i2;
                if(skipTexCoords) std::swap(i1, i2);
                if(i0) vertex.position = positions[i0-1].xyz();
                if(i1) vertex.texCoord = texCoords[i1-1].xy();
                if(i2) vertex.normal = normals[i2-1];
                vertices.push_back(vertex);                
            }

            for(size_t i=2; i<faceIndices.size(); ++i)
            {
                triangles.push_back({faceIndices[0], faceIndices[i-1], faceIndices[i]});
            }
        }
    }

    Mesh mesh;
    mesh.vertices = vertices;
    mesh.triangles = triangles;
    mesh.Upload();
    return mesh;
}

std::string LoadTextFile(const std::string & filename)
{
    std::string contents;
    FILE * f = fopen(filename.c_str(), "rb");
    if(f)
    {
        fseek(f, 0, SEEK_END);
        auto len = ftell(f);
        contents.resize(len);
        fseek(f, 0, SEEK_SET);
        fread(&contents[0], 1, contents.size(), f);
        fclose(f);
    }
    return contents;
}