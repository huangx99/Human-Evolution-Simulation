#include "sim/world/world_state.h"
#include "sim/system/cognitive_memory_system.h"
#include "sim/cognitive/memory_consolidation.h"
#include "sim/runtime/simulation_hash.h"
#include "rules/human_evolution/human_evolution_rule_pack.h"
#include "test_framework.h"

namespace
{
FocusedStimulus MakeFocused(EntityId agentId,
                            ConceptTypeId concept,
                            SenseType sense,
                            Vec2i location,
                            u64 stimulusId,
                            Tick tick,
                            f32 score = 0.8f,
                            f32 confidence = 0.8f)
{
    FocusedStimulus focused;
    focused.stimulus.id = stimulusId;
    focused.stimulus.observerId = agentId;
    focused.stimulus.sense = sense;
    focused.stimulus.concept = concept;
    focused.stimulus.location = location;
    focused.stimulus.confidence = confidence;
    focused.stimulus.tick = tick;
    focused.attentionScore = score;
    return focused;
}

void RunMemory(CognitiveMemorySystem& memorySystem,
               WorldState& world,
               const FocusedStimulus& focused,
               Tick tick)
{
    world.Cognitive().frameFocused.clear();
    world.Cognitive().frameMemories.clear();
    world.Sim().clock.currentTick = tick;
    world.Cognitive().frameFocused.push_back(focused);
    SystemContext ctx(world);
    memorySystem.Update(ctx);
    world.events.Dispatch();
}
}

TEST(spatial_memory_merge_same_concept_nearby)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& concepts = rp.GetHumanEvolutionContext().concepts;
    EntityId agentId = world.SpawnAgent(5, 5);
    CognitiveMemorySystem memorySystem;

    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {5, 5}, 100, 1, 0.7f), 1);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {7, 6}, 101, 2, 0.9f), 2);

    const auto& memories = world.Cognitive().GetAgentMemories(agentId);
    ASSERT_EQ(memories.size(), 1u);
    ASSERT_EQ(memories[0].reinforcementCount, 2u);
    ASSERT_EQ(memories[0].location.x, 7);
    ASSERT_EQ(memories[0].location.y, 6);
    ASSERT_EQ(memories[0].sourceStimulusId, 101u);

    return true;
}

TEST(spatial_memory_does_not_merge_far_locations)
{
    HumanEvolutionRulePack rp;
    WorldState world(32, 32, 42, rp);
    const auto& concepts = rp.GetHumanEvolutionContext().concepts;
    EntityId agentId = world.SpawnAgent(5, 5);
    CognitiveMemorySystem memorySystem;

    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {5, 5}, 100, 1), 1);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {12, 12}, 101, 2), 2);

    ASSERT_EQ(world.Cognitive().GetAgentMemories(agentId).size(), 2u);
    ASSERT_EQ(world.Cognitive().nextMemoryId, 3u);

    return true;
}

TEST(spatial_memory_does_not_merge_different_concepts)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& concepts = rp.GetHumanEvolutionContext().concepts;
    EntityId agentId = world.SpawnAgent(5, 5);
    CognitiveMemorySystem memorySystem;

    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {5, 5}, 100, 1), 1);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.food, SenseType::Vision, {6, 5}, 101, 2), 2);

    ASSERT_EQ(world.Cognitive().GetAgentMemories(agentId).size(), 2u);

    return true;
}

TEST(spatial_memory_does_not_merge_different_senses)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& concepts = rp.GetHumanEvolutionContext().concepts;
    EntityId agentId = world.SpawnAgent(5, 5);
    CognitiveMemorySystem memorySystem;

    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {5, 5}, 100, 1), 1);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Thermal, {6, 5}, 101, 2), 2);

    const auto& memories = world.Cognitive().GetAgentMemories(agentId);
    ASSERT_EQ(memories.size(), 2u);
    ASSERT_TRUE(memories[0].sense != memories[1].sense);

    return true;
}

TEST(spatial_memory_does_not_merge_outside_time_window)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& concepts = rp.GetHumanEvolutionContext().concepts;
    EntityId agentId = world.SpawnAgent(5, 5);
    CognitiveMemorySystem memorySystem;

    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {5, 5}, 100, 1), 1);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {6, 5}, 101, 20), 20);

    ASSERT_EQ(world.Cognitive().GetAgentMemories(agentId).size(), 2u);

    return true;
}

TEST(spatial_memory_merge_does_not_consume_next_memory_id)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& concepts = rp.GetHumanEvolutionContext().concepts;
    EntityId agentId = world.SpawnAgent(5, 5);
    world.Cognitive().nextMemoryId = 9;
    CognitiveMemorySystem memorySystem;

    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {5, 5}, 100, 1), 1);
    ASSERT_EQ(world.Cognitive().nextMemoryId, 10u);

    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {6, 5}, 101, 2), 2);

    const auto& memories = world.Cognitive().GetAgentMemories(agentId);
    ASSERT_EQ(memories.size(), 1u);
    ASSERT_EQ(memories[0].id, 9u);
    ASSERT_EQ(world.Cognitive().nextMemoryId, 10u);

    return true;
}

TEST(spatial_memory_reinforcement_count_increments)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& concepts = rp.GetHumanEvolutionContext().concepts;
    EntityId agentId = world.SpawnAgent(5, 5);
    CognitiveMemorySystem memorySystem;

    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {5, 5}, 100, 1), 1);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {6, 5}, 101, 2), 2);

    ASSERT_EQ(world.Cognitive().GetAgentMemories(agentId)[0].reinforcementCount, 2u);

    return true;
}

TEST(memory_consolidation_formed_reinforced_and_stabilized_events)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& concepts = rp.GetHumanEvolutionContext().concepts;
    EntityId agentId = world.SpawnAgent(5, 5);
    CognitiveMemorySystem memorySystem;

    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {5, 5}, 100, 1, 0.8f, 0.8f), 1);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {6, 5}, 101, 2, 0.8f, 0.8f), 2);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {6, 6}, 102, 3, 0.8f, 0.8f), 3);

    ASSERT_EQ(world.events.GetArchive(EventType::CognitiveMemoryFormed).size(), 1u);
    ASSERT_EQ(world.events.GetArchive(EventType::CognitiveMemoryReinforced).size(), 1u);
    ASSERT_EQ(world.events.GetArchive(EventType::CognitiveMemoryStabilized).size(), 1u);
    ASSERT_EQ(world.Cognitive().GetAgentMemories(agentId)[0].kind, MemoryKind::Stable);

    return true;
}

TEST(smell_memory_uses_larger_merge_radius)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& concepts = rp.GetHumanEvolutionContext().concepts;
    EntityId agentId = world.SpawnAgent(5, 5);
    CognitiveMemorySystem memorySystem;

    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.food, SenseType::Smell, {5, 5}, 100, 1), 1);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.food, SenseType::Smell, {9, 5}, 101, 2), 2);

    ASSERT_EQ(world.Cognitive().GetAgentMemories(agentId).size(), 1u);

    return true;
}

TEST(touch_memory_uses_smaller_merge_radius)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& concepts = rp.GetHumanEvolutionContext().concepts;
    EntityId agentId = world.SpawnAgent(5, 5);
    CognitiveMemorySystem memorySystem;

    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Touch, {5, 5}, 100, 1), 1);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Touch, {8, 5}, 101, 2), 2);

    ASSERT_EQ(world.Cognitive().GetAgentMemories(agentId).size(), 2u);

    return true;
}

TEST(social_memory_uses_social_merge_radius)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& concepts = rp.GetHumanEvolutionContext().concepts;
    EntityId agentId = world.SpawnAgent(5, 5);
    CognitiveMemorySystem memorySystem;

    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fear, SenseType::Social, {5, 5}, 100, 1), 1);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fear, SenseType::Social, {10, 5}, 101, 2), 2);

    ASSERT_EQ(world.Cognitive().GetAgentMemories(agentId).size(), 1u);

    return true;
}


TEST(sound_memory_uses_larger_merge_radius)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& concepts = rp.GetHumanEvolutionContext().concepts;
    EntityId agentId = world.SpawnAgent(5, 5);
    CognitiveMemorySystem memorySystem;

    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fear, SenseType::Sound, {5, 5}, 100, 1), 1);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fear, SenseType::Sound, {10, 5}, 101, 2), 2);

    ASSERT_EQ(world.Cognitive().GetAgentMemories(agentId).size(), 1u);

    return true;
}

TEST(stable_memory_reinforces_instead_of_creating_short_term_satellite)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& concepts = rp.GetHumanEvolutionContext().concepts;
    EntityId agentId = world.SpawnAgent(5, 5);
    CognitiveMemorySystem memorySystem;

    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {5, 5}, 100, 1, 0.8f, 0.8f), 1);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {6, 5}, 101, 2, 0.8f, 0.8f), 2);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {6, 6}, 102, 3, 0.8f, 0.8f), 3);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {6, 6}, 103, 4, 0.8f, 0.8f), 4);

    const auto& memories = world.Cognitive().GetAgentMemories(agentId);
    ASSERT_EQ(memories.size(), 1u);
    ASSERT_EQ(memories[0].kind, MemoryKind::Stable);
    ASSERT_EQ(memories[0].reinforcementCount, 4u);

    return true;
}


TEST(stable_memory_kind_changes_full_hash)
{
    HumanEvolutionRulePack rpA;
    HumanEvolutionRulePack rpB;
    WorldState a(16, 16, 42, rpA);
    WorldState b(16, 16, 42, rpB);
    const auto& concepts = rpA.GetHumanEvolutionContext().concepts;
    EntityId agentA = a.SpawnAgent(5, 5);
    EntityId agentB = b.SpawnAgent(5, 5);

    MemoryRecord shortTerm;
    shortTerm.id = 1;
    shortTerm.ownerId = agentA;
    shortTerm.kind = MemoryKind::ShortTerm;
    shortTerm.subject = concepts.fire;
    shortTerm.sense = SenseType::Vision;
    shortTerm.location = {5, 5};
    shortTerm.strength = 0.8f;
    shortTerm.confidence = 0.8f;
    shortTerm.createdTick = 1;
    shortTerm.lastReinforcedTick = 3;
    shortTerm.reinforcementCount = 3;
    shortTerm.sourceStimulusId = 102;

    MemoryRecord stable = shortTerm;
    stable.ownerId = agentB;
    stable.kind = MemoryKind::Stable;

    a.Cognitive().GetAgentMemories(agentA).push_back(shortTerm);
    b.Cognitive().GetAgentMemories(agentB).push_back(stable);

    ASSERT_TRUE(ComputeWorldHash(a, HashTier::Full) != ComputeWorldHash(b, HashTier::Full));

    return true;
}

TEST(memory_consolidation_same_inputs_produce_same_full_hash)
{
    HumanEvolutionRulePack rpA;
    HumanEvolutionRulePack rpB;
    WorldState a(16, 16, 42, rpA);
    WorldState b(16, 16, 42, rpB);
    const auto& concepts = rpA.GetHumanEvolutionContext().concepts;
    EntityId agentA = a.SpawnAgent(5, 5);
    EntityId agentB = b.SpawnAgent(5, 5);
    CognitiveMemorySystem memoryA;
    CognitiveMemorySystem memoryB;

    for (u64 i = 0; i < 3; i++)
    {
        RunMemory(memoryA, a,
                  MakeFocused(agentA, concepts.fire, SenseType::Vision,
                              {static_cast<i32>(5 + i), 5}, 100 + i, 1 + i, 0.8f, 0.8f), 1 + i);
        RunMemory(memoryB, b,
                  MakeFocused(agentB, concepts.fire, SenseType::Vision,
                              {static_cast<i32>(5 + i), 5}, 100 + i, 1 + i, 0.8f, 0.8f), 1 + i);
    }

    ASSERT_EQ(ComputeWorldHash(a, HashTier::Full), ComputeWorldHash(b, HashTier::Full));

    return true;
}

TEST(stable_memory_reinforces_nearby_same_concept)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& concepts = rp.GetHumanEvolutionContext().concepts;
    EntityId agentId = world.SpawnAgent(5, 5);
    CognitiveMemorySystem memorySystem;

    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {5, 5}, 100, 1, 0.8f, 0.8f), 1);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {6, 5}, 101, 2, 0.8f, 0.8f), 2);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {6, 6}, 102, 3, 0.8f, 0.8f), 3);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {7, 6}, 103, 20, 0.9f, 0.8f), 20);

    const auto& memories = world.Cognitive().GetAgentMemories(agentId);
    ASSERT_EQ(memories.size(), 1u);
    ASSERT_EQ(memories[0].kind, MemoryKind::Stable);
    ASSERT_EQ(memories[0].reinforcementCount, 4u);
    ASSERT_EQ(memories[0].sourceStimulusId, 103u);
    ASSERT_EQ(memories[0].location.x, 7);
    ASSERT_EQ(memories[0].location.y, 6);

    return true;
}

TEST(stable_memory_does_not_reinforce_far_location)
{
    HumanEvolutionRulePack rp;
    WorldState world(32, 32, 42, rp);
    const auto& concepts = rp.GetHumanEvolutionContext().concepts;
    EntityId agentId = world.SpawnAgent(5, 5);
    CognitiveMemorySystem memorySystem;

    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {5, 5}, 100, 1, 0.8f, 0.8f), 1);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {6, 5}, 101, 2, 0.8f, 0.8f), 2);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {6, 6}, 102, 3, 0.8f, 0.8f), 3);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {20, 20}, 103, 20, 0.9f, 0.8f), 20);

    ASSERT_EQ(world.Cognitive().GetAgentMemories(agentId).size(), 2u);

    return true;
}

TEST(stable_memory_does_not_reinforce_different_concept)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& concepts = rp.GetHumanEvolutionContext().concepts;
    EntityId agentId = world.SpawnAgent(5, 5);
    CognitiveMemorySystem memorySystem;

    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {5, 5}, 100, 1, 0.8f, 0.8f), 1);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {6, 5}, 101, 2, 0.8f, 0.8f), 2);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {6, 6}, 102, 3, 0.8f, 0.8f), 3);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.food, SenseType::Vision, {6, 6}, 103, 20, 0.9f, 0.8f), 20);

    ASSERT_EQ(world.Cognitive().GetAgentMemories(agentId).size(), 2u);

    return true;
}

TEST(stable_memory_reinforcement_does_not_consume_next_memory_id)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& concepts = rp.GetHumanEvolutionContext().concepts;
    EntityId agentId = world.SpawnAgent(5, 5);
    CognitiveMemorySystem memorySystem;

    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {5, 5}, 100, 1, 0.8f, 0.8f), 1);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {6, 5}, 101, 2, 0.8f, 0.8f), 2);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {6, 6}, 102, 3, 0.8f, 0.8f), 3);
    const u64 nextBefore = world.Cognitive().nextMemoryId;
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {7, 6}, 103, 20, 0.9f, 0.8f), 20);

    ASSERT_EQ(world.Cognitive().GetAgentMemories(agentId).size(), 1u);
    ASSERT_EQ(world.Cognitive().nextMemoryId, nextBefore);

    return true;
}

TEST(stable_memory_reinforcement_emits_reinforced_event)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& concepts = rp.GetHumanEvolutionContext().concepts;
    EntityId agentId = world.SpawnAgent(5, 5);
    CognitiveMemorySystem memorySystem;

    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {5, 5}, 100, 1, 0.8f, 0.8f), 1);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {6, 5}, 101, 2, 0.8f, 0.8f), 2);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {6, 6}, 102, 3, 0.8f, 0.8f), 3);
    RunMemory(memorySystem, world,
              MakeFocused(agentId, concepts.fire, SenseType::Vision, {7, 6}, 103, 20, 0.9f, 0.8f), 20);

    ASSERT_EQ(world.events.GetArchive(EventType::CognitiveMemoryFormed).size(), 1u);
    ASSERT_EQ(world.events.GetArchive(EventType::CognitiveMemoryStabilized).size(), 1u);
    ASSERT_EQ(world.events.GetArchive(EventType::CognitiveMemoryReinforced).size(), 2u);

    return true;
}

TEST(memory_merge_unions_result_and_context_tags_without_duplicates)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& concepts = rp.GetHumanEvolutionContext().concepts;
    EntityId agentId = world.SpawnAgent(5, 5);
    auto& memories = world.Cognitive().GetAgentMemories(agentId);

    MemoryRecord existing;
    existing.id = 1;
    existing.ownerId = agentId;
    existing.kind = MemoryKind::ShortTerm;
    existing.subject = concepts.fire;
    existing.sense = SenseType::Vision;
    existing.location = {5, 5};
    existing.strength = 0.7f;
    existing.confidence = 0.7f;
    existing.lastReinforcedTick = 1;
    existing.contextTags = {concepts.smoke};
    existing.resultTags = {concepts.pain};
    memories.push_back(existing);

    MemoryRecord incoming = existing;
    incoming.sourceStimulusId = 101;
    incoming.contextTags = {concepts.smoke, concepts.heat};
    incoming.resultTags = {concepts.pain, concepts.fear};

    MemoryConsolidationConfig config;
    ASSERT_TRUE(MemoryConsolidation::TryMergeMemory(memories, incoming, 2, config) != nullptr);

    ASSERT_EQ(memories[0].contextTags.size(), 2u);
    ASSERT_EQ(memories[0].resultTags.size(), 2u);

    return true;
}

TEST(memory_merge_keeps_stronger_emotional_weight)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& concepts = rp.GetHumanEvolutionContext().concepts;
    EntityId agentId = world.SpawnAgent(5, 5);
    auto& memories = world.Cognitive().GetAgentMemories(agentId);

    MemoryRecord existing;
    existing.id = 1;
    existing.ownerId = agentId;
    existing.kind = MemoryKind::ShortTerm;
    existing.subject = concepts.fire;
    existing.sense = SenseType::Vision;
    existing.location = {5, 5};
    existing.strength = 0.7f;
    existing.emotionalWeight = -0.8f;
    existing.confidence = 0.7f;
    existing.lastReinforcedTick = 1;
    memories.push_back(existing);

    MemoryRecord incoming = existing;
    incoming.emotionalWeight = -0.2f;

    MemoryConsolidationConfig config;
    ASSERT_TRUE(MemoryConsolidation::TryMergeMemory(memories, incoming, 2, config) != nullptr);
    ASSERT_NEAR(memories[0].emotionalWeight, -0.8f, 0.001f);

    return true;
}

TEST(memory_merge_allows_stronger_incoming_emotional_weight)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& concepts = rp.GetHumanEvolutionContext().concepts;
    EntityId agentId = world.SpawnAgent(5, 5);
    auto& memories = world.Cognitive().GetAgentMemories(agentId);

    MemoryRecord existing;
    existing.id = 1;
    existing.ownerId = agentId;
    existing.kind = MemoryKind::ShortTerm;
    existing.subject = concepts.fire;
    existing.sense = SenseType::Vision;
    existing.location = {5, 5};
    existing.strength = 0.7f;
    existing.emotionalWeight = -0.2f;
    existing.confidence = 0.7f;
    existing.lastReinforcedTick = 1;
    memories.push_back(existing);

    MemoryRecord incoming = existing;
    incoming.emotionalWeight = -0.9f;

    MemoryConsolidationConfig config;
    ASSERT_TRUE(MemoryConsolidation::TryMergeMemory(memories, incoming, 2, config) != nullptr);
    ASSERT_NEAR(memories[0].emotionalWeight, -0.9f, 0.001f);

    return true;
}
