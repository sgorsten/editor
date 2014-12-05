#include "asset.h"

#include <vector>
#include <iostream>

struct AssetLibrary::List
{
    std::function<std::shared_ptr<void>(const std::string & id)> loader;
    std::vector<std::shared_ptr<Record>> records;

    std::shared_ptr<Record> GetRecord(const std::string & id)
    {
        for(auto & record : records) if(record->id == id) return record;
    
        if(loader)
        {
            try
            {
                auto a = loader(id);
                auto r = std::make_shared<Record>();
                r->id = id;
                r->asset = move(a);
                records.push_back(r);
                return r;
            }
            catch(const std::exception & e)
            {
                std::cerr << "Unable to load asset \'" << id << "\': " << e.what() << std::endl;
                // TODO: Make some sort of record, so we only attempt to load an asset once
            }
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
