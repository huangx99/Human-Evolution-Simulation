#pragma once

#include "sim/ecology/ecology_entity.h"
#include "core/types/types.h"
#include <deque>
#include <string>

// Manages all ecology entities in the world.
// Uses deque so pointers/references remain valid across insertions.
class EcologyRegistry
{
public:
    // Create an entity with a base material
    EcologyEntity& Create(MaterialId material, const std::string& name = "")
    {
        entities.push_back({});
        auto& entity = entities.back();
        entity.id = nextId++;
        entity.material = material;
        entity.state = MaterialDB::GetDefaultState(material);
        entity.name = name;
        return entity;
    }

    // Find entity by ID
    EcologyEntity* Find(EntityId id)
    {
        for (auto& e : entities)
        {
            if (e.id == id) return &e;
        }
        return nullptr;
    }

    const EcologyEntity* Find(EntityId id) const
    {
        for (const auto& e : entities)
        {
            if (e.id == id) return &e;
        }
        return nullptr;
    }

    // Query: all entities at a position
    std::vector<EcologyEntity*> At(i32 x, i32 y)
    {
        std::vector<EcologyEntity*> result;
        for (auto& e : entities)
        {
            if (e.x == x && e.y == y) result.push_back(&e);
        }
        return result;
    }

    // Query: entities at a position that have a specific capability
    std::vector<EcologyEntity*> AtWithCapability(i32 x, i32 y, Capability cap)
    {
        std::vector<EcologyEntity*> result;
        for (auto& e : entities)
        {
            if (e.x == x && e.y == y && e.HasCapability(cap))
                result.push_back(&e);
        }
        return result;
    }

    // Query: entities at a position that have a specific affordance
    std::vector<EcologyEntity*> AtWithAffordance(i32 x, i32 y, Affordance aff)
    {
        std::vector<EcologyEntity*> result;
        for (auto& e : entities)
        {
            if (e.x == x && e.y == y && e.HasAffordance(aff))
                result.push_back(&e);
        }
        return result;
    }

    // Query: all entities with a specific capability
    std::vector<EcologyEntity*> WithCapability(Capability cap)
    {
        std::vector<EcologyEntity*> result;
        for (auto& e : entities)
        {
            if (e.HasCapability(cap)) result.push_back(&e);
        }
        return result;
    }

    // Query: all entities with a specific affordance
    std::vector<EcologyEntity*> WithAffordance(Affordance aff)
    {
        std::vector<EcologyEntity*> result;
        for (auto& e : entities)
        {
            if (e.HasAffordance(aff)) result.push_back(&e);
        }
        return result;
    }

    // Query: all entities with a specific state
    std::vector<EcologyEntity*> WithState(MaterialState s)
    {
        std::vector<EcologyEntity*> result;
        for (auto& e : entities)
        {
            if (e.HasState(s)) result.push_back(&e);
        }
        return result;
    }

    // Remove entity by ID
    bool Remove(EntityId id)
    {
        for (auto it = entities.begin(); it != entities.end(); ++it)
        {
            if (it->id == id)
            {
                entities.erase(it);
                return true;
            }
        }
        return false;
    }

    // Access all entities
    std::deque<EcologyEntity>& All() { return entities; }
    const std::deque<EcologyEntity>& All() const { return entities; }

    size_t Count() const { return entities.size(); }
    EntityId NextId() const { return nextId; }

    // State management for replay save/restore
    void Clear() { entities.clear(); }
    void InsertCopy(const EcologyEntity& e) { entities.push_back(e); }
    void SetNextId(EntityId id) { nextId = id; }

private:
    std::deque<EcologyEntity> entities;
    EntityId nextId = 1;
};
