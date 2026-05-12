#pragma once

#include "core/types/types.h"
#include "core/math/vec2i.h"
#include <vector>
#include <string>

// Memory entry: abstract representation of an experience
struct MemoryEntry
{
    Tick tick = 0;
    Vec2i location;
    f32 intensity = 0.0f;   // how strong/significant this memory is
    f32 emotional = 0.0f;   // emotional valence (-1 = bad, +1 = good)

    enum class Category : u8
    {
        None = 0,
        FireSeen,
        FoodFound,
        DangerFelt,
        SafePlace,
        Migration,
        Cooperation,
        Discovery
    };

    Category category = Category::None;
};

// Short-term memory: recent experiences, high detail, fast decay
struct ShortTermMemory
{
    static constexpr size_t MAX_ENTRIES = 16;
    std::vector<MemoryEntry> entries;

    void Add(const MemoryEntry& entry)
    {
        if (entries.size() >= MAX_ENTRIES)
        {
            // Remove oldest
            entries.erase(entries.begin());
        }
        entries.push_back(entry);
    }

    void Decay(f32 rate)
    {
        for (auto& e : entries)
        {
            e.intensity *= rate;
        }
        // Remove faded memories
        entries.erase(
            std::remove_if(entries.begin(), entries.end(),
                [](const MemoryEntry& e) { return e.intensity < 0.01f; }),
            entries.end());
    }

    const MemoryEntry* FindMostIntense() const
    {
        if (entries.empty()) return nullptr;
        const MemoryEntry* best = &entries[0];
        for (const auto& e : entries)
        {
            if (e.intensity > best->intensity) best = &e;
        }
        return best;
    }
};

// Long-term memory: consolidated experiences, lower detail, slow decay
struct LongTermMemory
{
    static constexpr size_t MAX_ENTRIES = 64;
    std::vector<MemoryEntry> entries;

    void Consolidate(const ShortTermMemory& stm, Tick currentTick)
    {
        for (const auto& stmEntry : stm.entries)
        {
            // Only consolidate strong memories
            if (stmEntry.intensity < 0.5f) continue;

            // Check if similar memory already exists
            bool found = false;
            for (auto& ltmEntry : entries)
            {
                if (ltmEntry.category == stmEntry.category &&
                    ltmEntry.location == stmEntry.location)
                {
                    // Reinforce existing memory
                    ltmEntry.intensity = std::min(1.0f, ltmEntry.intensity + 0.1f);
                    ltmEntry.tick = currentTick;
                    found = true;
                    break;
                }
            }

            if (!found && entries.size() < MAX_ENTRIES)
            {
                entries.push_back(stmEntry);
            }
        }
    }

    void Decay(f32 rate)
    {
        for (auto& e : entries)
        {
            e.intensity *= rate;
        }
        entries.erase(
            std::remove_if(entries.begin(), entries.end(),
                [](const MemoryEntry& e) { return e.intensity < 0.01f; }),
            entries.end());
    }
};

// Group memory: shared knowledge among a group of agents
struct GroupMemory
{
    Vec2i knownFoodSource;
    Vec2i knownDangerZone;
    f32 foodSourceIntensity = 0.0f;
    f32 dangerZoneIntensity = 0.0f;

    void UpdateFromAgent(const LongTermMemory& ltm)
    {
        for (const auto& entry : ltm.entries)
        {
            if (entry.category == MemoryEntry::Category::FoodFound &&
                entry.intensity > foodSourceIntensity)
            {
                knownFoodSource = entry.location;
                foodSourceIntensity = entry.intensity;
            }
            if (entry.category == MemoryEntry::Category::DangerFelt &&
                entry.intensity > dangerZoneIntensity)
            {
                knownDangerZone = entry.location;
                dangerZoneIntensity = entry.intensity;
            }
        }
    }

    void Decay(f32 rate)
    {
        foodSourceIntensity *= rate;
        dangerZoneIntensity *= rate;
    }
};

// Civilization memory: long-term cultural knowledge (future)
struct CivilizationMemory
{
    // Placeholder for future civilization-level memory
    // Examples: discovered technologies, cultural practices, migration routes
    std::vector<std::string> discoveries;
};
