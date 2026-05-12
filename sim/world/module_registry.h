#pragma once

#include <memory>
#include <unordered_map>
#include <typeindex>
#include <typeinfo>
#include <cassert>

class IModule
{
public:
    virtual ~IModule() = default;
};

class ModuleRegistry
{
public:
    template<typename T, typename... Args>
    T& Register(Args&&... args)
    {
        static_assert(std::is_base_of_v<IModule, T>, "T must derive from IModule");
        auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *ptr;
        modules[typeid(T)] = std::move(ptr);
        return ref;
    }

    template<typename T>
    T& Get()
    {
        static_assert(std::is_base_of_v<IModule, T>, "T must derive from IModule");
        auto it = modules.find(typeid(T));
        assert(it != modules.end() && "Module not registered");
        return static_cast<T&>(*it->second);
    }

    template<typename T>
    const T& Get() const
    {
        static_assert(std::is_base_of_v<IModule, T>, "T must derive from IModule");
        auto it = modules.find(typeid(T));
        assert(it != modules.end() && "Module not registered");
        return static_cast<const T&>(*it->second);
    }

    template<typename T>
    bool Has() const
    {
        return modules.count(typeid(T)) > 0;
    }

private:
    std::unordered_map<std::type_index, std::unique_ptr<IModule>> modules;
};
