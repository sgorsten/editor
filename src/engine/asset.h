#ifndef EDITOR_ASSET_H
#define EDITOR_ASSET_H

#include <memory>
#include <string>
#include <map>
#include <functional>
#include <typeindex>

class AssetLibrary
{
    struct Record
    {
        std::string id;
        std::shared_ptr<void> asset;
    };

    struct List
    {
        std::function<std::shared_ptr<void>(const std::string & id)> loader;
        std::map<std::string, std::shared_ptr<Record>> assets;
    };

    std::map<std::type_index, List> lists;
public:
    template<class T> class Handle
    {
        std::shared_ptr<const Record> record;
    public:
        Handle() {}
        Handle(const std::shared_ptr<const Record> & record) : record(record) {}

        bool IsValid() const { return record && record->asset; }
        const T & GetAsset() const { return *reinterpret_cast<const T *>(record->asset.get()); }
        const std::string & GetId() const { return record->id; }
    };

    template<class T, class F> void RegisterLoader(F load) { lists[typeid(T)].loader = [load](const std::string & id) { return std::make_shared<T>(load(id)); }; }

    template<class T> Handle<T> RegisterAsset(const std::string & id, T && asset) 
    { 
        auto r = std::make_shared<Record>();
        r->id = id;
        r->asset = std::make_shared<T>(std::move(asset));
        lists[typeid(T)].assets[id] = r;
        return r;
    }

    template<class T> Handle<T> GetAsset(const std::string & id)
    {
        auto it = lists.find(typeid(T));
        if(it != end(lists))
        {
            auto it2 = it->second.assets.find(id);
            if(it2 != end(it->second.assets)) return it2->second;

            if(it->second.loader)
            {
                auto r = std::make_shared<Record>();
                r->id = id;
                r->asset = it->second.loader(id);
                it->second.assets[id] = r;
                return r;
            }
        }
        return {};
    }
};

#endif