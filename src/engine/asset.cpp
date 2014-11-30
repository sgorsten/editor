#include "asset.h"

#include <vector>

struct AssetLibrary::List
{
    std::function<std::shared_ptr<void>(const std::string & id)> loader;
    std::vector<std::shared_ptr<Record>> records;

    std::shared_ptr<Record> GetRecord(const std::string & id)
    {
        for(auto & record : records) if(record->id == id) return record;
    
        if(loader)
        {
            auto r = std::make_shared<Record>();
            r->id = id;
            r->asset = loader(id);
            records.push_back(r);
            return r;
        }

        return {};
    }
};

AssetLibrary::AssetLibrary() {}
AssetLibrary::~AssetLibrary() {}

void AssetLibrary::SetLoader(const std::type_info & type, std::function<std::shared_ptr<void>(const std::string & id)> loader)
{
    lists[type].loader = loader;
}

std::shared_ptr<AssetLibrary::Record> AssetLibrary::AddAsset(const std::type_info & type, const std::string & id, std::shared_ptr<void> asset)
{ 
    auto r = std::make_shared<Record>();
    r->id = id;
    r->asset = asset;
    lists[type].records.push_back(r);
    return r;
}

std::shared_ptr<AssetLibrary::Record> AssetLibrary::GetAsset(const std::type_info & type, const std::string & id)
{
    auto it = lists.find(type);
    if(it == end(lists)) return {};
    return it->second.GetRecord(id);
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