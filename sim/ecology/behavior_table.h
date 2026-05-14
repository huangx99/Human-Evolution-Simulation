#pragma once

// BehaviorTable: single source of truth for all entity behaviors.
//
// Every material+state combination can register callbacks for:
//   - SenseEmission (objective sensory signals)
//   - Concept inference (what concepts does this entity represent)
//   - Food effect (what happens when an agent eats it)
//   - Use effect (what happens when an agent uses it)
//
// All downstream systems (Perception, FoodAssessor, FoodSourceSystem)
// query this table instead of duplicating logic.

#include "core/types/types.h"
#include "sim/ecology/material_id.h"
#include "sim/ecology/material_state.h"
#include "sim/ecology/sense_emission.h"
#include "sim/cognitive/concept_id.h"
#include <functional>
#include <vector>
#include <string>
#include <algorithm>

struct Agent;

enum class FoodValidity : u8 { NotFood, EdibleFresh, EdibleRisky, Spoiled };

struct FoodEffect
{
    FoodValidity validity = FoodValidity::NotFood;
    f32 nutrition = 0.0f;
    f32 risk = 0.0f;
    bool consumable = false;
};

struct UseEffect
{
    bool usable = false;
    f32 effectiveness = 0.0f;
};

struct BehaviorInput
{
    MaterialId material;
    MaterialState requiredState = MaterialState::None;
    i32 count = 1;
};

enum class LocationType : u8 { Anywhere, Ground, Container, Workbench, Shrine };

struct BehaviorContext
{
    LocationType location = LocationType::Anywhere;
    MaterialId containerType = MaterialId{};
};

struct BehaviorOutput
{
    MaterialId material;             // None = in-place transform
    MaterialState addState = MaterialState::None;
    MaterialState removeState = MaterialState::None;
};

struct BehaviorCallbacks
{
    SenseEmission emission{};

    std::function<void(const Agent&, std::vector<ConceptTypeId>&)> inferConcepts;
    std::function<FoodEffect(const Agent&)> onEat;
    std::function<UseEffect(const Agent&, const Agent&)> onUse;
    std::function<f32(const Agent&)> successRate;
    std::function<bool(const Agent&)> canDiscover;
};

struct BehaviorEntry
{
    std::string id;
    std::vector<BehaviorInput> inputs;
    BehaviorContext context;
    Tick duration = 0;
    f32 probability = 1.0f;
    BehaviorOutput output;
    BehaviorCallbacks callbacks;
};

class BehaviorTable
{
public:
    static BehaviorTable& Instance()
    {
        static BehaviorTable table;
        return table;
    }

    void Register(BehaviorEntry entry)
    {
        entries_.push_back(std::move(entry));
    }

    // Find the entry that defines this material+state as its output.
    const BehaviorEntry* FindByOutput(MaterialId material, MaterialState state) const
    {
        // Burning entities are universally inedible
        if (HasState(state, MaterialState::Burning))
            return nullptr;

        for (const auto& r : entries_)
        {
            // In-place transform: output.material == None, match input material
            if (r.output.material == MaterialId{})
            {
                if (r.inputs.empty()) continue;
                bool materialMatch = false;
                for (const auto& inp : r.inputs)
                {
                    if (inp.material == material) { materialMatch = true; break; }
                }
                if (!materialMatch) continue;
            }
            else
            {
                if (r.output.material != material) continue;
            }

            // Check addState: entity must have all bits in addState
            if (r.output.addState != MaterialState::None)
            {
                if ((static_cast<u32>(state) & static_cast<u32>(r.output.addState))
                    != static_cast<u32>(r.output.addState))
                    continue;
            }
            // Check removeState: entity must NOT have any bit in removeState
            if (r.output.removeState != MaterialState::None)
            {
                if (static_cast<u32>(state) & static_cast<u32>(r.output.removeState))
                    continue;
            }
            return &r;
        }
        return nullptr;
    }

    // Find entries that consume this material+state as input.
    std::vector<const BehaviorEntry*> FindMatchingInputs(
        MaterialId material, MaterialState state, LocationType loc) const
    {
        std::vector<const BehaviorEntry*> result;
        for (const auto& r : entries_)
        {
            if (r.inputs.empty()) continue;
            if (r.context.location != LocationType::Anywhere &&
                r.context.location != loc) continue;
            for (const auto& inp : r.inputs)
            {
                if (inp.material == material &&
                    (inp.requiredState == MaterialState::None ||
                     (static_cast<u32>(state) & static_cast<u32>(inp.requiredState))))
                {
                    result.push_back(&r);
                    break;
                }
            }
        }
        return result;
    }

    SenseEmission GetEmission(MaterialId material, MaterialState state) const
    {
        auto* r = FindByOutput(material, state);
        if (r) return r->callbacks.emission;
        return {};
    }

    FoodEffect GetFoodEffect(MaterialId material, MaterialState state,
                             const Agent& agent) const
    {
        auto* r = FindByOutput(material, state);
        if (r && r->callbacks.onEat) return r->callbacks.onEat(agent);
        return {FoodValidity::NotFood, 0.0f, 0.0f, false};
    }

    void GetConcepts(MaterialId material, MaterialState state,
                     const Agent& agent, std::vector<ConceptTypeId>& out) const
    {
        auto* r = FindByOutput(material, state);
        if (r && r->callbacks.inferConcepts) r->callbacks.inferConcepts(agent, out);
    }

    size_t EntryCount() const { return entries_.size(); }

    void Clear() { entries_.clear(); }

private:
    BehaviorTable() = default;
    std::vector<BehaviorEntry> entries_;
};

// Legacy aliases for backward compatibility
using RecipeTable = BehaviorTable;
using RecipeEntry = BehaviorEntry;
using RecipeCallbacks = BehaviorCallbacks;
using RecipeInput = BehaviorInput;
using RecipeOutput = BehaviorOutput;
using RecipeContext = BehaviorContext;
