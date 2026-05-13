#pragma once

#include "sim/field/field_id.h"
#include "core/container/field2d.h"
#include "sim/world/module_registry.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

// FieldKind: distinguishes 2D spatial grids from scalar global values.

enum class FieldKind : u8 { Spatial, Scalar };

// FieldEntry: one registered field in the FieldModule.
// Owns its Field2D via unique_ptr (safe across vector reallocation).

struct FieldEntry
{
    FieldKey key;
    std::string name;
    FieldKind kind;
    std::unique_ptr<Field2D<f32>> spatial;  // non-null when kind == Spatial
    f32 scalarValue = 0.0f;                  // used when kind == Scalar
};

// FieldModule: dynamic field registry and owner.
//
// All simulation fields (temperature, fire, smell, etc.) are registered here.
// FieldModule OWNS the Field2D instances. EnvironmentModule/InformationModule
// become thin wrappers that reference FieldModule data during the transition.
//
// FieldIndex is a compact 1-based index assigned at registration time.
// FieldKey is a stable hash of the namespaced field name, used for persistence.

class FieldModule : public IModule
{
public:
    FieldModule(i32 w, i32 h) : width_(w), height_(h) {}

    // --- Registration ---

    // Register a spatial (2D grid) field. Returns the assigned FieldIndex.
    FieldIndex RegisterField(FieldKey key, const char* name, f32 defaultValue)
    {
        auto entry = FieldEntry{};
        entry.key = key;
        entry.name = name;
        entry.kind = FieldKind::Spatial;
        entry.spatial = std::make_unique<Field2D<f32>>(width_, height_, defaultValue);

        u16 id = static_cast<u16>(entries_.size() + 1);  // 1-based
        entries_.push_back(std::move(entry));
        keyToIndex_[key.value] = id;
        nameToIndex_[entries_.back().name] = id;
        return FieldIndex(id);
    }

    // Register a scalar (global value) field. Returns the assigned FieldIndex.
    FieldIndex RegisterScalarField(FieldKey key, const char* name, f32 defaultValue)
    {
        auto entry = FieldEntry{};
        entry.key = key;
        entry.name = name;
        entry.kind = FieldKind::Scalar;
        entry.scalarValue = defaultValue;

        u16 id = static_cast<u16>(entries_.size() + 1);
        entries_.push_back(std::move(entry));
        keyToIndex_[key.value] = id;
        nameToIndex_[entries_.back().name] = id;
        return FieldIndex(id);
    }

    // --- Runtime access ---

    f32 Read(FieldIndex id, i32 x, i32 y) const
    {
        const auto& e = entries_[id.index - 1];
        if (e.kind == FieldKind::Spatial)
            return e.spatial->At(x, y);
        return e.scalarValue;
    }

    void WriteNext(FieldIndex id, i32 x, i32 y, f32 value)
    {
        auto& e = entries_[id.index - 1];
        if (e.kind == FieldKind::Spatial)
            e.spatial->WriteNext(x, y) = value;
        else
            e.scalarValue = value;
    }

    // Returns a mutable reference into the next buffer (for FieldRef aliasing)
    f32& WriteNextRef(FieldIndex id, i32 x, i32 y)
    {
        auto& e = entries_[id.index - 1];
        if (e.kind == FieldKind::Spatial)
            return e.spatial->WriteNext(x, y);
        return e.scalarValue;
    }

    bool InBounds(FieldIndex id, i32 x, i32 y) const
    {
        const auto& e = entries_[id.index - 1];
        if (e.kind == FieldKind::Scalar)
            return true;  // scalar is always in bounds
        return e.spatial->InBounds(x, y);
    }

    void CopyCurrentToNext(FieldIndex id)
    {
        auto& e = entries_[id.index - 1];
        if (e.kind == FieldKind::Spatial)
            e.spatial->CopyCurrentToNext();
    }

    // Fill both current and next buffers (for test setup / reset)
    void FillBoth(FieldIndex id, f32 value)
    {
        auto& e = entries_[id.index - 1];
        if (e.kind == FieldKind::Spatial && e.spatial)
        {
            e.spatial->current.Fill(value);
            e.spatial->next.Fill(value);
        }
        else
        {
            e.scalarValue = value;
        }
    }

    // --- Lookup ---

    FieldIndex FindByKey(FieldKey key) const
    {
        auto it = keyToIndex_.find(key.value);
        if (it != keyToIndex_.end())
            return FieldIndex(it->second);
        return FieldIndex(0);
    }

    FieldIndex FindByName(const char* name) const
    {
        auto it = nameToIndex_.find(name);
        if (it != nameToIndex_.end())
            return FieldIndex(it->second);
        return FieldIndex(0);
    }

    FieldKey GetKey(FieldIndex id) const { return entries_[id.index - 1].key; }
    FieldKind GetKind(FieldIndex id) const { return entries_[id.index - 1].kind; }
    const std::string& GetName(FieldIndex id) const { return entries_[id.index - 1].name; }
    u16 FieldCount() const { return static_cast<u16>(entries_.size()); }

    // --- Double-buffer ---

    void SwapAll()
    {
        for (auto& e : entries_)
        {
            if (e.kind == FieldKind::Spatial && e.spatial)
                e.spatial->Swap();
        }
    }

    // Swap a single field's double buffers (for FieldRef::Swap)
    void SwapField(FieldIndex id)
    {
        auto& e = entries_[id.index - 1];
        if (e.kind == FieldKind::Spatial && e.spatial)
            e.spatial->Swap();
    }

    // --- Iteration (for hash, snapshot, replay) ---

    const std::vector<FieldEntry>& Entries() const { return entries_; }

    // Direct Field2D access (for hash/snapshot that need raw grid data)
    Field2D<f32>* GetField2D(FieldIndex id)
    {
        auto& e = entries_[id.index - 1];
        return e.spatial.get();
    }

    const Field2D<f32>* GetField2D(FieldIndex id) const
    {
        const auto& e = entries_[id.index - 1];
        return e.spatial.get();
    }

private:
    i32 width_, height_;
    std::vector<FieldEntry> entries_;
    std::unordered_map<u64, u16> keyToIndex_;
    std::unordered_map<std::string, u16> nameToIndex_;
};
