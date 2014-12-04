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

    struct List;
    std::map<std::type_index, List> lists;

    void SetLoader(const std::type_info & type, std::function<std::shared_ptr<void>(const std::string & id)> loader);
    std::shared_ptr<Record> AddAsset(const std::type_info & type, const std::string & id, std::shared_ptr<void> asset);
    std::shared_ptr<Record> GetAsset(const std::type_info & type, const std::string & id);
public:
    AssetLibrary();
    ~AssetLibrary();

    template<class T> class Handle
    {
        std::shared_ptr<const Record> record;
    public:
        Handle() {}
        Handle(const std::shared_ptr<const Record> & record) : record(record) {}

        operator bool () const { return record && record->asset; }
        bool operator == (const Handle & r) const { return record == r.record; }
        bool operator != (const Handle & r) const { return record != r.record; }

        bool operator ! () const { return !record || !record->asset; }
        const T & operator * () const { return *reinterpret_cast<const T *>(record->asset.get()); }
        const T * operator -> () const { return reinterpret_cast<const T *>(record->asset.get()); }

        const std::string & GetId() const { static const std::string empty; return record ? record->id : empty; }
    };

    template<class T, class F> void SetLoader(F load) { SetLoader(typeid(T), [load](const std::string & id) { return std::make_shared<T>(load(id)); }); }
    template<class T> Handle<T> AddAsset(const std::string & id, T && asset) { return AddAsset(typeid(T), id, std::make_shared<T>(std::move(asset))); }
    template<class T> Handle<T> GetAsset(const std::string & id) { return GetAsset(typeid(T), id); }
};

#endif