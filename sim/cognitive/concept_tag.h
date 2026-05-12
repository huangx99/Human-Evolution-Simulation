#pragma once

#include "core/types/types.h"
#include <array>
#include <bitset>

// ConceptTag: the fundamental unit of cognitive meaning.
//
// ARCHITECTURE NOTE: ConceptTag is a cognitive semantic ID, NOT a gameplay enum.
// It represents what an agent perceives, remembers, or believes — not what
// objectively exists. "Fire" as a ConceptTag means "the agent's mental model
// of fire", which may be incomplete or wrong.
//
// Tags are grouped into SemanticGroup flags so that related concepts can be
// discovered and associated. Example: Fire belongs to {Environment, Danger,
// Light, Heat}. An agent who fears fire may also fear other Danger-group
// concepts, and an agent who values fire's warmth may value other Heat-group
// concepts.
//
// RULES:
// 1. Never use raw strings or ad-hoc u32 for cognitive concepts.
// 2. All concepts must be registered here with their semantic groups.
// 3. New tags must serve cognitive relations (hypotheses, knowledge edges).
//    Don't add tags that only categorize items — that's what MaterialId is for.
// 4. Tags should be abstract enough to support polysemy: Fire means different
//    things to different agents (warmth vs danger vs light source).

enum class ConceptTag : u32
{
    None = 0,

    // === Natural phenomena ===
    Fire,
    Water,
    Wind,
    Rain,
    Lightning,
    Darkness,
    Light,
    Smoke,
    Ice,
    Heat,
    Cold,

    // === Materials ===
    Wood,
    Stone,
    Grass,
    Meat,
    Bone,
    Ash,
    Coal,
    Fruit,

    // === States / conditions ===
    Warmth,
    Wetness,
    Dryness,
    Hunger,
    Pain,
    Satiety,
    Health,
    Death,

    // === Danger ===
    Danger,
    Beast,
    Predator,
    Fall,
    Drowning,
    Burning,

    // === Opportunities ===
    Food,
    Shelter,
    Tool,
    Path,

    // === Social ===
    Companion,
    Stranger,
    Group,
    Signal,
    Voice,

    // === Abstract (emerge from experience) ===
    Safety,
    Comfort,
    Fear,
    Curiosity,
    Trust,

    Count
};

// Semantic groups: each concept can belong to multiple groups.
// Groups define cognitive affinity — concepts in the same group
// are more likely to be associated in memory and knowledge.

enum class SemanticGroup : u32
{
    None        = 0,

    Environment = 1 << 0,
    Danger      = 1 << 1,
    Light       = 1 << 2,
    Heat        = 1 << 3,
    Cold_       = 1 << 4,   // trailing underscore avoids macro collision
    Moisture    = 1 << 5,
    Organic     = 1 << 6,
    Mineral     = 1 << 7,
    Edible      = 1 << 8,
    Threat      = 1 << 9,
    Shelter_    = 1 << 10,
    Social      = 1 << 11,
    Tool_       = 1 << 12,
    Abstract    = 1 << 13,
};

constexpr SemanticGroup operator|(SemanticGroup a, SemanticGroup b)
{
    return static_cast<SemanticGroup>(static_cast<u32>(a) | static_cast<u32>(b));
}

inline bool HasGroup(SemanticGroup flags, SemanticGroup g)
{
    return (static_cast<u32>(flags) & static_cast<u32>(g)) != 0;
}

// Static registry: maps ConceptTag → semantic groups
namespace ConceptRegistry
{
    struct ConceptInfo
    {
        SemanticGroup groups = SemanticGroup::None;
        const char* name = "";
    };

    constexpr std::array<ConceptInfo, static_cast<size_t>(ConceptTag::Count)> Init()
    {
        std::array<ConceptInfo, static_cast<size_t>(ConceptTag::Count)> db{};

        // Natural phenomena
        db[static_cast<size_t>(ConceptTag::Fire)]     = { SemanticGroup::Environment | SemanticGroup::Danger | SemanticGroup::Light | SemanticGroup::Heat, "Fire" };
        db[static_cast<size_t>(ConceptTag::Water)]    = { SemanticGroup::Environment | SemanticGroup::Moisture | SemanticGroup::Organic, "Water" };
        db[static_cast<size_t>(ConceptTag::Wind)]     = { SemanticGroup::Environment, "Wind" };
        db[static_cast<size_t>(ConceptTag::Rain)]     = { SemanticGroup::Environment | SemanticGroup::Moisture | SemanticGroup::Cold_, "Rain" };
        db[static_cast<size_t>(ConceptTag::Lightning)] = { SemanticGroup::Environment | SemanticGroup::Danger | SemanticGroup::Light, "Lightning" };
        db[static_cast<size_t>(ConceptTag::Darkness)] = { SemanticGroup::Environment | SemanticGroup::Danger, "Darkness" };
        db[static_cast<size_t>(ConceptTag::Light)]    = { SemanticGroup::Environment | SemanticGroup::Light, "Light" };
        db[static_cast<size_t>(ConceptTag::Smoke)]    = { SemanticGroup::Environment | SemanticGroup::Danger | SemanticGroup::Heat, "Smoke" };
        db[static_cast<size_t>(ConceptTag::Ice)]      = { SemanticGroup::Environment | SemanticGroup::Cold_ | SemanticGroup::Mineral, "Ice" };
        db[static_cast<size_t>(ConceptTag::Heat)]     = { SemanticGroup::Environment | SemanticGroup::Heat, "Heat" };
        db[static_cast<size_t>(ConceptTag::Cold)]     = { SemanticGroup::Environment | SemanticGroup::Cold_ | SemanticGroup::Danger, "Cold" };

        // Materials
        db[static_cast<size_t>(ConceptTag::Wood)]     = { SemanticGroup::Organic | SemanticGroup::Mineral | SemanticGroup::Tool_, "Wood" };
        db[static_cast<size_t>(ConceptTag::Stone)]    = { SemanticGroup::Mineral | SemanticGroup::Tool_, "Stone" };
        db[static_cast<size_t>(ConceptTag::Grass)]    = { SemanticGroup::Organic | SemanticGroup::Edible, "Grass" };
        db[static_cast<size_t>(ConceptTag::Meat)]     = { SemanticGroup::Organic | SemanticGroup::Edible, "Meat" };
        db[static_cast<size_t>(ConceptTag::Bone)]     = { SemanticGroup::Organic | SemanticGroup::Mineral | SemanticGroup::Tool_, "Bone" };
        db[static_cast<size_t>(ConceptTag::Ash)]      = { SemanticGroup::Mineral, "Ash" };
        db[static_cast<size_t>(ConceptTag::Coal)]     = { SemanticGroup::Mineral | SemanticGroup::Heat, "Coal" };
        db[static_cast<size_t>(ConceptTag::Fruit)]    = { SemanticGroup::Organic | SemanticGroup::Edible, "Fruit" };

        // States
        db[static_cast<size_t>(ConceptTag::Warmth)]   = { SemanticGroup::Heat | SemanticGroup::Abstract, "Warmth" };
        db[static_cast<size_t>(ConceptTag::Wetness)]  = { SemanticGroup::Moisture, "Wetness" };
        db[static_cast<size_t>(ConceptTag::Dryness)]  = { SemanticGroup::Environment, "Dryness" };
        db[static_cast<size_t>(ConceptTag::Hunger)]   = { SemanticGroup::Organic | SemanticGroup::Danger, "Hunger" };
        db[static_cast<size_t>(ConceptTag::Pain)]     = { SemanticGroup::Danger | SemanticGroup::Abstract, "Pain" };
        db[static_cast<size_t>(ConceptTag::Satiety)]  = { SemanticGroup::Organic | SemanticGroup::Abstract, "Satiety" };
        db[static_cast<size_t>(ConceptTag::Health)]   = { SemanticGroup::Organic | SemanticGroup::Abstract, "Health" };
        db[static_cast<size_t>(ConceptTag::Death)]    = { SemanticGroup::Danger | SemanticGroup::Abstract, "Death" };

        // Danger
        db[static_cast<size_t>(ConceptTag::Danger)]   = { SemanticGroup::Danger | SemanticGroup::Threat | SemanticGroup::Abstract, "Danger" };
        db[static_cast<size_t>(ConceptTag::Beast)]    = { SemanticGroup::Danger | SemanticGroup::Threat | SemanticGroup::Organic, "Beast" };
        db[static_cast<size_t>(ConceptTag::Predator)] = { SemanticGroup::Danger | SemanticGroup::Threat | SemanticGroup::Organic, "Predator" };
        db[static_cast<size_t>(ConceptTag::Fall)]     = { SemanticGroup::Danger | SemanticGroup::Threat, "Fall" };
        db[static_cast<size_t>(ConceptTag::Drowning)] = { SemanticGroup::Danger | SemanticGroup::Threat | SemanticGroup::Moisture, "Drowning" };
        db[static_cast<size_t>(ConceptTag::Burning)]  = { SemanticGroup::Danger | SemanticGroup::Heat, "Burning" };

        // Opportunities
        db[static_cast<size_t>(ConceptTag::Food)]     = { SemanticGroup::Edible | SemanticGroup::Organic | SemanticGroup::Abstract, "Food" };
        db[static_cast<size_t>(ConceptTag::Shelter)]  = { SemanticGroup::Shelter_ | SemanticGroup::Abstract, "Shelter" };
        db[static_cast<size_t>(ConceptTag::Tool)]     = { SemanticGroup::Tool_ | SemanticGroup::Abstract, "Tool" };
        db[static_cast<size_t>(ConceptTag::Path)]     = { SemanticGroup::Environment | SemanticGroup::Abstract, "Path" };

        // Social
        db[static_cast<size_t>(ConceptTag::Companion)] = { SemanticGroup::Social | SemanticGroup::Abstract, "Companion" };
        db[static_cast<size_t>(ConceptTag::Stranger)]  = { SemanticGroup::Social | SemanticGroup::Danger | SemanticGroup::Abstract, "Stranger" };
        db[static_cast<size_t>(ConceptTag::Group)]     = { SemanticGroup::Social | SemanticGroup::Abstract, "Group" };
        db[static_cast<size_t>(ConceptTag::Signal)]    = { SemanticGroup::Social | SemanticGroup::Abstract, "Signal" };
        db[static_cast<size_t>(ConceptTag::Voice)]     = { SemanticGroup::Social | SemanticGroup::Abstract, "Voice" };

        // Abstract
        db[static_cast<size_t>(ConceptTag::Safety)]    = { SemanticGroup::Abstract | SemanticGroup::Shelter_, "Safety" };
        db[static_cast<size_t>(ConceptTag::Comfort)]   = { SemanticGroup::Abstract | SemanticGroup::Heat, "Comfort" };
        db[static_cast<size_t>(ConceptTag::Fear)]      = { SemanticGroup::Abstract | SemanticGroup::Danger, "Fear" };
        db[static_cast<size_t>(ConceptTag::Curiosity)] = { SemanticGroup::Abstract, "Curiosity" };
        db[static_cast<size_t>(ConceptTag::Trust)]     = { SemanticGroup::Abstract | SemanticGroup::Social, "Trust" };

        return db;
    }

    constexpr auto concepts = Init();

    inline const ConceptInfo& Get(ConceptTag tag)
    {
        return concepts[static_cast<size_t>(tag)];
    }

    inline const char* GetName(ConceptTag tag)
    {
        return Get(tag).name;
    }

    inline SemanticGroup GetGroups(ConceptTag tag)
    {
        return Get(tag).groups;
    }

    inline bool InSameGroup(ConceptTag a, ConceptTag b)
    {
        return (static_cast<u32>(GetGroups(a)) & static_cast<u32>(GetGroups(b))) != 0;
    }
}
