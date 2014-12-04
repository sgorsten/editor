#ifndef ENGINE_PACK_H
#define ENGINE_PACK_H

#include "json.h"
#include "linalg.h"
#include "transform.h"
#include "asset.h"

class JsonSerializer
{
    struct Visitor { JsonSerializer & j; JsonObject & value; template<class U> void operator() (const char * name, const U & field) { auto val = j.Save(field); if(!val.isNull()) value.push_back({name, val}); } };
public:
    JsonSerializer() {}

    JsonValue Save(const bool & object) { return object; }
    JsonValue Save(const std::string & object) { return object; }
    template<class T> std::enable_if_t<std::is_arithmetic<T>::value, JsonValue> Save(const T & object) { return object; }
    template<class T> JsonValue Save(const vec<T,2> & object) { return JsonArray{object.x, object.y}; }
    template<class T> JsonValue Save(const vec<T,3> & object) { return JsonArray{object.x, object.y, object.z}; }
    template<class T> JsonValue Save(const vec<T,4> & object) { return JsonArray{object.x, object.y, object.z, object.w}; }
    JsonValue Save(const Pose & object) { return JsonArray{Save(object.position), Save(object.orientation)}; }
    template<class T> JsonValue Save(const std::unique_ptr<T> & object) { return object ? Save(*object) : nullptr; }
    template<class T> JsonValue Save(const std::shared_ptr<T> & object) { return object ? Save(*object) : nullptr; }
    template<class T> JsonValue Save(const std::vector<T> & object) { JsonArray a; for(auto & elem : object) a.push_back(Save(elem)); return a; }
    template<class T> JsonValue Save(const AssetLibrary::Handle<T> & object) { return object ? object.GetId() : nullptr; }
    template<class T> std::enable_if_t<std::is_class<T>::value, JsonValue> Save(const T & object) { JsonObject o; VisitFields((T&)object, Visitor{*this,o}); return o; }
};

class JsonDeserializer
{
    struct Visitor { JsonDeserializer & j; const JsonValue & value; template<class U> void operator() (const char * name, U & field) { j.Load(field, value[name]); } };
    AssetLibrary & assets;
public:
    JsonDeserializer(AssetLibrary & assets) : assets(assets) {}

    void Load(bool & object, const JsonValue & value) { object = value.isTrue(); }
    void Load(std::string & object, const JsonValue & value) { object = value.string(); }
    template<class T> std::enable_if_t<std::is_arithmetic<T>::value, void> Load(T & object, const JsonValue & value) { object = value.number<T>(); } 
    template<class T> void Load(vec<T,2> & object, const JsonValue & value) { Load(object.x, value[0]); Load(object.y, value[1]); }
    template<class T> void Load(vec<T,3> & object, const JsonValue & value) { Load(object.x, value[0]); Load(object.y, value[1]); Load(object.z, value[2]); }
    template<class T> void Load(vec<T,4> & object, const JsonValue & value) { Load(object.x, value[0]); Load(object.y, value[1]); Load(object.z, value[2]); Load(object.w, value[3]); }
    void Load(Pose & object, const JsonValue & value) { Load(object.position, value[0]); Load(object.orientation, value[1]); }
    template<class T> void Load(std::unique_ptr<T> & object, const JsonValue & value) { if(value.isObject()) { object = std::make_unique<T>(); Load(*object, value); } else object.reset(); }
    template<class T> void Load(std::shared_ptr<T> & object, const JsonValue & value) { if(value.isObject()) { object = std::make_shared<T>(); Load(*object, value); } else object.reset(); }
    template<class T> void Load(std::vector<T> & object, const JsonValue & value) { object.clear(); object.resize(value.array().size()); for(size_t i=0; i<object.size(); ++i) Load(object[i], value[i]); }
    template<class T> void Load(AssetLibrary::Handle<T> & object, const JsonValue & value) { object = assets.GetAsset<T>(value.string()); }
    template<class T> std::enable_if_t<std::is_class<T>::value, void> Load(T & object, const JsonValue & value) { VisitFields(object, Visitor{*this,value}); }
};

template<class T> JsonValue SerializeToJson(const T & object)
{
    return JsonSerializer().Save(object);
}

template<class T> T DeserializeFromJson(const JsonValue & value, AssetLibrary & assets)
{
    T object;
    JsonDeserializer(assets).Load(object, value);
    return object;
}

#endif
