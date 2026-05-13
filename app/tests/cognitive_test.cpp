#include "rules/human_evolution/human_evolution_rule_pack.h"
#include "test_framework.h"
#include "sim/world/world_state.h"
#include "sim/scheduler/scheduler.h"
#include "sim/scheduler/phase.h"
#include "sim/cognitive/concept_tag.h"

static HumanEvolutionRulePack g_rulePack;
static const auto& g_envCtx = g_rulePack.GetHumanEvolutionContext().environment;

// ============================================================
// P0-1: Subjective Perception
// ============================================================
TEST(cognitive_perception_filtering)
{
    WorldState world(32, 32, 42);
    world.Init(g_rulePack);

    // Place fire at center
    world.Fields().WriteNext(g_envCtx.fire, 16, 16, 80.0f);
    world.Fields().SwapAll();

    // Agent A: far from fire (corner)
    world.SpawnAgent(1, 1);
    // Agent B: near fire
    world.SpawnAgent(16, 15);

    world.RebuildSpatial();

    auto scheduler = CreateCognitiveScheduler(g_envCtx);
    scheduler.Tick(world);

    auto& cog = world.Cognitive();

    // Agent B (near fire) should have fire stimulus
    bool agentBHasFire = false;
    for (const auto& s : cog.frameStimuli)
    {
        if (s.observerId == 2 && s.concept == ConceptTag::Fire)
        {
            agentBHasFire = true;
            break;
        }
    }
    ASSERT_TRUE(agentBHasFire);

    // Agent A (far from fire) should NOT have fire stimulus
    bool agentAHasFire = false;
    for (const auto& s : cog.frameStimuli)
    {
        if (s.observerId == 1 && s.concept == ConceptTag::Fire)
        {
            agentAHasFire = true;
            break;
        }
    }
    ASSERT_TRUE(!agentAHasFire);

    return true;
}

// ============================================================
// P0-2: Attention Filtering
// ============================================================
TEST(cognitive_attention_filtering)
{
    WorldState world(32, 32, 42);
    world.Init(g_rulePack);

    // Create fire and food smell near agent (write to next, then swap once)
    world.Fields().WriteNext(g_envCtx.fire, 16, 16, 60.0f);
    for (i32 dy = -2; dy <= 2; dy++)
    {
        for (i32 dx = -2; dx <= 2; dx++)
        {
            i32 x = 16 + dx;
            i32 y = 16 + dy;
            if (world.Fields().InBounds(g_envCtx.smell, x, y))
                world.Fields().WriteNext(g_envCtx.smell, x, y, 30.0f);
        }
    }
    world.Fields().SwapAll();

    // Agent near both fire and food smell, very hungry
    auto agentId = world.SpawnAgent(16, 15);
    auto* agent = world.Agents().Find(agentId);
    agent->hunger = 90.0f;

    world.RebuildSpatial();

    auto scheduler = CreateCognitiveScheduler(g_envCtx);
    scheduler.Tick(world);

    auto& cog = world.Cognitive();

    ASSERT_TRUE(cog.frameStimuli.size() > 1);

    size_t focusedCount = 0;
    for (const auto& f : cog.frameFocused)
    {
        if (f.stimulus.observerId == agentId)
            focusedCount++;
    }
    ASSERT_TRUE(focusedCount <= 4);

    bool foodFocused = false;
    for (const auto& f : cog.frameFocused)
    {
        if (f.stimulus.observerId == agentId &&
            f.stimulus.concept == ConceptTag::Food)
        {
            foodFocused = true;
            break;
        }
    }
    ASSERT_TRUE(foodFocused);

    size_t memCount = 0;
    for (const auto& m : cog.frameMemories)
    {
        if (m.ownerId == agentId)
            memCount++;
    }
    ASSERT_TRUE(memCount <= focusedCount);

    return true;
}

// ============================================================
// P0-4: Forgetting
// ============================================================
TEST(cognitive_memory_decay)
{
    WorldState world(32, 32, 42);
    world.Init(g_rulePack);

    // Small fire
    world.Fields().WriteNext(g_envCtx.fire, 16, 16, 10.0f);
    world.Fields().SwapAll();

    world.SpawnAgent(16, 15);
    world.RebuildSpatial();

    auto scheduler = CreateCognitiveScheduler(g_envCtx);

    for (i32 i = 0; i < 5; i++)
        scheduler.Tick(world);

    auto& cog = world.Cognitive();
    auto& mems = cog.GetAgentMemories(1);
    ASSERT_TRUE(mems.size() > 0);

    size_t initialCount = mems.size();
    f32 peakFireStrength = 0.0f;
    for (const auto& m : mems)
    {
        if (m.subject == ConceptTag::Fire && m.strength > peakFireStrength)
            peakFireStrength = m.strength;
    }
    ASSERT_TRUE(peakFireStrength > 0.0f);

    // Remove fire and clear all stimuli
    world.Fields().FillBoth(g_envCtx.fire, 0.0f);
    world.Fields().FillBoth(g_envCtx.temperature, 20.0f);
    world.Fields().FillBoth(g_envCtx.smell, 0.0f);
    world.Fields().FillBoth(g_envCtx.danger, 0.0f);
    world.Fields().FillBoth(g_envCtx.smoke, 0.0f);

    for (i32 i = 0; i < 100; i++)
    {
        world.Fields().FillBoth(g_envCtx.fire, 0.0f);
        world.Fields().FillBoth(g_envCtx.temperature, 20.0f);
        world.Fields().FillBoth(g_envCtx.smell, 0.0f);
        world.Fields().FillBoth(g_envCtx.danger, 0.0f);
        world.Fields().FillBoth(g_envCtx.smoke, 0.0f);
        scheduler.Tick(world);
    }

    auto& memsAfter = cog.GetAgentMemories(1);

    ASSERT_TRUE(memsAfter.size() <= initialCount);

    for (const auto& m : memsAfter)
    {
        if (m.subject == ConceptTag::Fire)
        {
            ASSERT_TRUE(m.strength < peakFireStrength);
        }
    }

    return true;
}

// ============================================================
// P0-5: Hypothesis Formation
// ============================================================
TEST(cognitive_hypothesis_formation)
{
    WorldState world(32, 32, 42);
    world.Init(g_rulePack);

    world.Fields().WriteNext(g_envCtx.fire, 16, 16, 80.0f);
    world.Fields().SwapAll();

    world.SpawnAgent(16, 15);
    world.RebuildSpatial();

    auto scheduler = CreateCognitiveScheduler(g_envCtx);

    for (i32 i = 0; i < 100; i++)
    {
        if (i % 10 == 0)
        {
            world.Fields().WriteNext(g_envCtx.fire, 16, 16, 80.0f);
            world.Fields().SwapAll();
        }
        scheduler.Tick(world);
    }

    auto& cog = world.Cognitive();
    auto& hypotheses = cog.GetAgentHypotheses(1);

    ASSERT_TRUE(hypotheses.size() > 0);

    bool hasFireHypothesis = false;
    for (const auto& h : hypotheses)
    {
        if (h.causeConcept == ConceptTag::Fire || h.effectConcept == ConceptTag::Fire ||
            h.causeConcept == ConceptTag::Heat || h.effectConcept == ConceptTag::Heat)
        {
            hasFireHypothesis = true;
            break;
        }
    }
    ASSERT_TRUE(hasFireHypothesis);

    return true;
}

// ============================================================
// P0-6: Knowledge Graph Polysemy
// ============================================================
TEST(cognitive_knowledge_polysemy)
{
    // === Agent A: near intense fire ===
    {
        WorldState world(32, 32, 42);
        world.Init(g_rulePack);

        world.Fields().WriteNext(g_envCtx.fire, 16, 16, 100.0f);
        world.Fields().WriteNext(g_envCtx.fire, 17, 16, 80.0f);
        world.Fields().WriteNext(g_envCtx.fire, 15, 16, 80.0f);
        world.Fields().SwapAll();

        world.SpawnAgent(16, 16);
        world.RebuildSpatial();

        auto scheduler = CreateCognitiveScheduler(g_envCtx);

        for (i32 i = 0; i < 30; i++)
        {
            if (i % 10 == 0)
            {
                world.Fields().WriteNext(g_envCtx.fire, 16, 16, 100.0f);
                world.Fields().SwapAll();
            }
            scheduler.Tick(world);
        }

        auto& cog = world.Cognitive();
        auto& mems = cog.GetAgentMemories(1);

        f32 avgFireEmotion = 0.0f;
        u32 fireMemCount = 0;
        for (const auto& m : mems)
        {
            if (m.subject == ConceptTag::Fire)
            {
                avgFireEmotion += m.emotionalWeight;
                fireMemCount++;
            }
        }
        ASSERT_TRUE(fireMemCount > 0);
        avgFireEmotion /= fireMemCount;
        ASSERT_TRUE(avgFireEmotion < 0.0f);
    }

    // === Agent B: near mild fire ===
    {
        WorldState world(32, 32, 42);
        world.Init(g_rulePack);

        world.Fields().WriteNext(g_envCtx.fire, 16, 16, 25.0f);
        world.Fields().SwapAll();

        world.SpawnAgent(16, 14);
        world.RebuildSpatial();

        auto scheduler = CreateCognitiveScheduler(g_envCtx);

        for (i32 i = 0; i < 30; i++)
        {
            if (i % 10 == 0)
            {
                world.Fields().WriteNext(g_envCtx.fire, 16, 16, 25.0f);
                world.Fields().SwapAll();
            }
            scheduler.Tick(world);
        }

        auto& cog = world.Cognitive();
        auto& mems = cog.GetAgentMemories(1);

        f32 avgFireEmotion = 0.0f;
        u32 fireMemCount = 0;
        for (const auto& m : mems)
        {
            if (m.subject == ConceptTag::Fire)
            {
                avgFireEmotion += m.emotionalWeight;
                fireMemCount++;
            }
        }

        if (fireMemCount > 0)
        {
            avgFireEmotion /= fireMemCount;
            ASSERT_TRUE(avgFireEmotion >= -0.5f);
        }
    }

    return true;
}

// ============================================================
// P1-7: Wrong Knowledge
// ============================================================
TEST(cognitive_wrong_knowledge)
{
    // === Agent A: near fire ===
    WorldState worldA(32, 32, 42);
    worldA.Init(g_rulePack);
    worldA.Fields().WriteNext(g_envCtx.fire, 16, 16, 80.0f);
    worldA.Fields().SwapAll();
    worldA.SpawnAgent(16, 15);
    worldA.RebuildSpatial();

    auto schedulerA = CreateCognitiveScheduler(g_envCtx);
    for (i32 i = 0; i < 40; i++)
    {
        if (i % 10 == 0)
        {
            worldA.Fields().WriteNext(g_envCtx.fire, 16, 16, 80.0f);
            worldA.Fields().SwapAll();
        }
        schedulerA.Tick(worldA);
    }

    auto& hypsA = worldA.Cognitive().GetAgentHypotheses(1);
    ASSERT_TRUE(hypsA.size() > 0);

    // === Agent B: no fire, near corpse smell ===
    WorldState worldB(32, 32, 42);
    worldB.Init(g_rulePack);
    auto& corpse = worldB.Ecology().entities.Create(MaterialId::Flesh, "corpse");
    corpse.x = 16; corpse.y = 16;
    corpse.state = MaterialState::Dead;
    worldB.RebuildSpatial();

    worldB.SpawnAgent(16, 15);

    auto schedulerB = CreateCognitiveScheduler(g_envCtx);
    for (i32 i = 0; i < 40; i++)
        schedulerB.Tick(worldB);

    auto& hypsB = worldB.Cognitive().GetAgentHypotheses(1);

    bool aHasFire = false;
    for (const auto& h : hypsA)
    {
        if (h.causeConcept == ConceptTag::Fire || h.effectConcept == ConceptTag::Fire)
        {
            aHasFire = true;
            break;
        }
    }
    ASSERT_TRUE(aHasFire);

    bool bHasFire = false;
    for (const auto& h : hypsB)
    {
        if (h.causeConcept == ConceptTag::Fire || h.effectConcept == ConceptTag::Fire)
        {
            bHasFire = true;
            break;
        }
    }
    ASSERT_TRUE(!bHasFire);

    return true;
}

// ============================================================
// P1-8: Knowledge Reinforcement
// ============================================================
TEST(cognitive_knowledge_reinforcement)
{
    WorldState world(32, 32, 42);
    world.Init(g_rulePack);

    world.Fields().WriteNext(g_envCtx.fire, 16, 16, 80.0f);
    world.Fields().SwapAll();

    world.SpawnAgent(16, 15);
    world.RebuildSpatial();

    auto scheduler = CreateCognitiveScheduler(g_envCtx);

    for (i32 i = 0; i < 50; i++)
    {
        if (i % 10 == 0)
        {
            world.Fields().WriteNext(g_envCtx.fire, 16, 16, 80.0f);
            world.Fields().SwapAll();
        }
        scheduler.Tick(world);
    }

    auto& cog = world.Cognitive();
    auto& hypotheses = cog.GetAgentHypotheses(1);

    if (hypotheses.empty()) return true;

    f32 initialConfidence = hypotheses[0].confidence;
    u32 initialEvidence = hypotheses[0].supportingCount;

    for (i32 i = 0; i < 50; i++)
    {
        if (i % 10 == 0)
        {
            world.Fields().WriteNext(g_envCtx.fire, 16, 16, 80.0f);
            world.Fields().SwapAll();
        }
        scheduler.Tick(world);
    }

    ASSERT_TRUE(hypotheses[0].confidence >= initialConfidence ||
                hypotheses[0].supportingCount >= initialEvidence);

    return true;
}

// ============================================================
// P2-9: Individual Difference
// ============================================================
TEST(cognitive_individual_difference)
{
    WorldState world(32, 32, 42);
    world.Init(g_rulePack);

    world.Fields().WriteNext(g_envCtx.fire, 16, 16, 80.0f);
    world.Fields().SwapAll();

    world.SpawnAgent(16, 15);
    world.SpawnAgent(1, 1);

    world.RebuildSpatial();

    auto scheduler = CreateCognitiveScheduler(g_envCtx);

    for (i32 i = 0; i < 80; i++)
    {
        if (i % 10 == 0)
        {
            world.Fields().WriteNext(g_envCtx.fire, 16, 16, 80.0f);
            world.Fields().SwapAll();
        }
        scheduler.Tick(world);
    }

    auto& cog = world.Cognitive();

    auto& memsA = cog.GetAgentMemories(1);
    auto& memsB = cog.GetAgentMemories(2);

    bool aHasFire = false;
    for (const auto& m : memsA)
    {
        if (m.subject == ConceptTag::Fire) { aHasFire = true; break; }
    }
    ASSERT_TRUE(aHasFire);

    bool bHasFire = false;
    for (const auto& m : memsB)
    {
        if (m.subject == ConceptTag::Fire) { bHasFire = true; break; }
    }
    ASSERT_TRUE(!bHasFire);

    auto nodesA = cog.knowledgeGraph.FindAgentNodes(1);
    auto nodesB = cog.knowledgeGraph.FindAgentNodes(2);

    ASSERT_TRUE(nodesA.size() >= nodesB.size());

    return true;
}

// ============================================================
// Bonus: Full pipeline integration test
// ============================================================
TEST(cognitive_full_pipeline)
{
    WorldState world(32, 32, 42);
    world.Init(g_rulePack);

    world.Fields().WriteNext(g_envCtx.fire, 16, 16, 80.0f);
    world.Fields().SwapAll();

    world.SpawnAgent(16, 15);
    world.RebuildSpatial();

    auto scheduler = CreateCognitiveScheduler(g_envCtx);

    for (i32 i = 0; i < 150; i++)
    {
        if (i % 10 == 0)
        {
            world.Fields().WriteNext(g_envCtx.fire, 16, 16, 80.0f);
            world.Fields().SwapAll();
        }
        scheduler.Tick(world);
    }

    auto& cog = world.Cognitive();

    auto& mems = cog.GetAgentMemories(1);
    ASSERT_TRUE(mems.size() > 0);

    auto& hyps = cog.GetAgentHypotheses(1);
    ASSERT_TRUE(hyps.size() > 0);

    auto nodes = cog.knowledgeGraph.FindAgentNodes(1);
    ASSERT_TRUE(nodes.size() > 0);

    size_t edgeCount = 0;
    for (const auto* n : nodes)
    {
        auto edges = cog.knowledgeGraph.FindEdgesFrom(1, 0, n->concept);
        edgeCount += edges.size();
    }

    return true;
}

// ============================================================
// C-1: Different agents form different knowledge graphs
// ============================================================
TEST(cognitive_different_knowledge_graphs)
{
    WorldState world(32, 32, 42);
    world.Init(g_rulePack);

    world.Fields().WriteNext(g_envCtx.fire, 16, 16, 80.0f);
    world.Fields().SwapAll();

    auto& corpse = world.Ecology().entities.Create(MaterialId::Flesh, "corpse");
    corpse.x = 8; corpse.y = 8;
    corpse.state = MaterialState::Dead;
    world.RebuildSpatial();

    world.SpawnAgent(16, 15);
    world.SpawnAgent(8, 7);

    auto scheduler = CreateCognitiveScheduler(g_envCtx);

    for (i32 i = 0; i < 100; i++)
    {
        if (i % 10 == 0)
        {
            world.Fields().WriteNext(g_envCtx.fire, 16, 16, 80.0f);
            world.Fields().SwapAll();
        }
        scheduler.Tick(world);
    }

    auto& cog = world.Cognitive();

    auto& memsA = cog.GetAgentMemories(1);
    auto& memsB = cog.GetAgentMemories(2);
    ASSERT_TRUE(memsA.size() > 0);
    ASSERT_TRUE(memsB.size() > 0);

    bool aHasFire = false;
    for (const auto& m : memsA)
    {
        if (m.subject == ConceptTag::Fire) { aHasFire = true; break; }
    }
    ASSERT_TRUE(aHasFire);

    bool bHasFire = false;
    for (const auto& m : memsB)
    {
        if (m.subject == ConceptTag::Fire) { bHasFire = true; break; }
    }
    ASSERT_TRUE(!bHasFire);

    auto& hypsA = cog.GetAgentHypotheses(1);
    auto& hypsB = cog.GetAgentHypotheses(2);

    bool aFireHyp = false;
    for (const auto& h : hypsA)
    {
        if (h.causeConcept == ConceptTag::Fire || h.effectConcept == ConceptTag::Fire)
        { aFireHyp = true; break; }
    }
    ASSERT_TRUE(aFireHyp);

    auto nodesA = cog.knowledgeGraph.FindAgentNodes(1);
    auto nodesB = cog.knowledgeGraph.FindAgentNodes(2);

    bool aFireNode = false;
    for (const auto* n : nodesA)
    {
        if (n->concept == ConceptTag::Fire) { aFireNode = true; break; }
    }
    ASSERT_TRUE(aFireNode);

    return true;
}

// ============================================================
// C-2: Fire → Danger confidence increases with repeated burns
// ============================================================
TEST(cognitive_fire_danger_confidence)
{
    WorldState world(32, 32, 42);
    world.Init(g_rulePack);

    world.Fields().WriteNext(g_envCtx.fire, 16, 16, 100.0f);
    world.Fields().WriteNext(g_envCtx.fire, 17, 16, 80.0f);
    world.Fields().SwapAll();

    world.SpawnAgent(16, 16);
    world.RebuildSpatial();

    auto scheduler = CreateCognitiveScheduler(g_envCtx);

    for (i32 i = 0; i < 200; i++)
    {
        if (i % 10 == 0)
        {
            world.Fields().WriteNext(g_envCtx.fire, 16, 16, 100.0f);
            world.Fields().WriteNext(g_envCtx.fire, 17, 16, 80.0f);
            world.Fields().SwapAll();
        }
        scheduler.Tick(world);
    }

    auto& cog = world.Cognitive();

    auto& hyps = cog.GetAgentHypotheses(1);
    ASSERT_TRUE(hyps.size() > 0);

    Hypothesis* fireHyp = nullptr;
    for (auto& h : hyps)
    {
        if (h.causeConcept == ConceptTag::Fire || h.effectConcept == ConceptTag::Fire)
        {
            fireHyp = &h;
            break;
        }
    }
    ASSERT_TRUE(fireHyp != nullptr);

    ASSERT_TRUE(fireHyp->confidence > 0.2f);
    ASSERT_TRUE(fireHyp->supportingCount > 1);

    return true;
}

// ============================================================
// C-3: SmokeMeansFireNearby knowledge boosts smoke attention
// ============================================================
TEST(cognitive_smoke_knowledge_boosts_attention)
{
    // Phase 1: baseline
    f32 baselineScore = 0.0f;
    {
        WorldState world(32, 32, 42);
        world.Init(g_rulePack);

        for (i32 dy = -2; dy <= 2; dy++)
            for (i32 dx = -2; dx <= 2; dx++)
                if (world.Fields().InBounds(g_envCtx.smoke, 16+dx, 16+dy))
                    world.Fields().WriteNext(g_envCtx.smoke, 16+dx, 16+dy, 30.0f);
        world.Fields().SwapAll();

        world.SpawnAgent(16, 16);
        world.RebuildSpatial();

        auto scheduler = CreateCognitiveScheduler(g_envCtx);
        scheduler.Tick(world);

        for (const auto& f : world.Cognitive().frameFocused)
        {
            if (f.stimulus.observerId == 1 && f.stimulus.concept == ConceptTag::Smoke)
            {
                baselineScore = f.attentionScore;
                break;
            }
        }
    }

    // Phase 2: with knowledge
    f32 knowledgeScore = 0.0f;
    {
        WorldState world(32, 32, 42);
        world.Init(g_rulePack);

        {
            auto& kg = world.Cognitive().knowledgeGraph;
            u64 smokeId = kg.GetOrCreateNode(1, 0, ConceptTag::Smoke, 0).id;
            u64 fireId = kg.GetOrCreateNode(1, 0, ConceptTag::Fire, 0).id;
            auto& edge = kg.GetOrCreateEdge(smokeId, fireId,
                                             KnowledgeRelation::Signals, 0);
            edge.confidence = 0.8f;
            edge.strength = 2.0f;
            edge.evidenceCount = 5;
        }

        for (i32 dy = -2; dy <= 2; dy++)
            for (i32 dx = -2; dx <= 2; dx++)
                if (world.Fields().InBounds(g_envCtx.smoke, 16+dx, 16+dy))
                    world.Fields().WriteNext(g_envCtx.smoke, 16+dx, 16+dy, 30.0f);
        world.Fields().SwapAll();

        world.SpawnAgent(16, 16);
        world.RebuildSpatial();

        auto scheduler = CreateCognitiveScheduler(g_envCtx);
        scheduler.Tick(world);

        for (const auto& f : world.Cognitive().frameFocused)
        {
            if (f.stimulus.observerId == 1 && f.stimulus.concept == ConceptTag::Smoke)
            {
                knowledgeScore = f.attentionScore;
                break;
            }
        }
    }

    ASSERT_TRUE(baselineScore > 0.0f);
    ASSERT_TRUE(knowledgeScore > 0.0f);
    ASSERT_TRUE(knowledgeScore > baselineScore);

    return true;
}

// ============================================================
// C-4: Unreinforced memories decay
// ============================================================
TEST(cognitive_memory_decay_unreinforced)
{
    WorldState world(32, 32, 42);
    world.Init(g_rulePack);

    world.Fields().WriteNext(g_envCtx.fire, 16, 16, 60.0f);
    world.Fields().SwapAll();

    world.SpawnAgent(16, 15);
    world.RebuildSpatial();

    auto scheduler = CreateCognitiveScheduler(g_envCtx);

    for (i32 i = 0; i < 5; i++)
        scheduler.Tick(world);

    auto& cog = world.Cognitive();
    auto& mems = cog.GetAgentMemories(1);
    ASSERT_TRUE(mems.size() > 0);

    f32 peakStrength = 0.0f;
    for (const auto& m : mems)
    {
        if (m.strength > peakStrength)
            peakStrength = m.strength;
    }
    ASSERT_TRUE(peakStrength > 0.0f);

    world.Fields().FillBoth(g_envCtx.fire, 0.0f);

    for (i32 i = 0; i < 100; i++)
    {
        world.Fields().FillBoth(g_envCtx.fire, 0.0f);
        scheduler.Tick(world);
    }

    auto& memsAfter = cog.GetAgentMemories(1);
    ASSERT_TRUE(memsAfter.size() <= mems.size());

    for (const auto& m : memsAfter)
    {
        ASSERT_TRUE(m.strength < peakStrength);
    }

    return true;
}

// ============================================================
// C-5: New rule added without changing DiscoverySystem
// ============================================================
TEST(cognitive_rule_driven_discovery)
{
    WorldState world(32, 32, 42);
    world.Init(g_rulePack);

    // Hot environment
    for (i32 dy = -1; dy <= 1; dy++)
    {
        for (i32 dx = -1; dx <= 1; dx++)
        {
            i32 x = 16 + dx;
            i32 y = 16 + dy;
            if (world.Fields().InBounds(g_envCtx.temperature, x, y))
                world.Fields().WriteNext(g_envCtx.temperature, x, y, 50.0f);
        }
    }
    world.Fields().SwapAll();

    // Also place fire
    world.Fields().WriteNext(g_envCtx.fire, 16, 16, 60.0f);
    world.Fields().SwapAll();

    world.SpawnAgent(16, 16);
    world.RebuildSpatial();

    auto scheduler = CreateCognitiveScheduler(g_envCtx);

    for (i32 i = 0; i < 100; i++)
    {
        if (i % 10 == 0)
        {
            world.Fields().WriteNext(g_envCtx.fire, 16, 16, 60.0f);
            world.Fields().SwapAll();
            for (i32 dy = -1; dy <= 1; dy++)
                for (i32 dx = -1; dx <= 1; dx++)
                    if (world.Fields().InBounds(g_envCtx.temperature, 16+dx, 16+dy))
                        world.Fields().WriteNext(g_envCtx.temperature, 16+dx, 16+dy, 50.0f);
            world.Fields().SwapAll();
        }
        scheduler.Tick(world);
    }

    auto& cog = world.Cognitive();
    auto& hyps = cog.GetAgentHypotheses(1);

    ASSERT_TRUE(hyps.size() > 0);

    bool hasRelevantHyp = false;
    for (const auto& h : hyps)
    {
        if (h.causeConcept == ConceptTag::Heat || h.effectConcept == ConceptTag::Heat ||
            h.causeConcept == ConceptTag::Danger || h.effectConcept == ConceptTag::Danger ||
            h.causeConcept == ConceptTag::Fire || h.effectConcept == ConceptTag::Fire)
        {
            hasRelevantHyp = true;
            break;
        }
    }
    ASSERT_TRUE(hasRelevantHyp);

    return true;
}

// ============================================================
// C-6: KnowledgeGraph debug dump
// ============================================================
TEST(cognitive_knowledge_debug_dump)
{
    WorldState world(32, 32, 42);
    world.Init(g_rulePack);

    world.Fields().WriteNext(g_envCtx.fire, 16, 16, 80.0f);
    world.Fields().SwapAll();

    world.SpawnAgent(16, 15);
    world.RebuildSpatial();

    auto scheduler = CreateCognitiveScheduler(g_envCtx);

    for (i32 i = 0; i < 150; i++)
    {
        if (i % 10 == 0)
        {
            world.Fields().WriteNext(g_envCtx.fire, 16, 16, 80.0f);
            world.Fields().SwapAll();
        }
        scheduler.Tick(world);
    }

    auto& cog = world.Cognitive();

    std::string dump = cog.knowledgeGraph.Dump(1);

    auto nodes = cog.knowledgeGraph.FindAgentNodes(1);
    bool hasEdges = false;
    for (const auto* n : nodes)
    {
        auto edges = cog.knowledgeGraph.FindEdgesFrom(1, 0, n->concept);
        if (!edges.empty()) { hasEdges = true; break; }
    }

    if (hasEdges)
    {
        ASSERT_TRUE(!dump.empty());
        ASSERT_TRUE(dump.find("->") != std::string::npos);
    }

    std::string dumpEmpty = cog.knowledgeGraph.Dump(999);
    ASSERT_TRUE(dumpEmpty.empty());

    return true;
}

// ============================================================
// C-7: Memory doesn't directly read WorldState
// ============================================================
TEST(cognitive_memory_from_focused_only)
{
    WorldState world(32, 32, 42);
    world.Init(g_rulePack);

    world.Fields().WriteNext(g_envCtx.fire, 16, 16, 80.0f);
    world.Fields().SwapAll();

    world.SpawnAgent(16, 15);
    world.RebuildSpatial();

    auto scheduler = CreateCognitiveScheduler(g_envCtx);
    scheduler.Tick(world);

    auto& cog = world.Cognitive();

    std::vector<u64> focusedIds;
    for (const auto& f : cog.frameFocused)
    {
        if (f.stimulus.observerId == 1)
            focusedIds.push_back(f.stimulus.id);
    }

    std::vector<MemoryRecord> tickMemories;
    for (const auto& m : cog.frameMemories)
    {
        if (m.ownerId == 1)
            tickMemories.push_back(m);
    }

    u32 stimuliCount = 0;
    for (const auto& s : cog.frameStimuli)
        if (s.observerId == 1) stimuliCount++;

    ASSERT_TRUE(stimuliCount >= focusedIds.size());
    ASSERT_TRUE(focusedIds.size() >= tickMemories.size());

    for (const auto& mem : tickMemories)
    {
        ASSERT_TRUE(mem.id > 0);
        ASSERT_TRUE(mem.subject != ConceptTag::None);

        bool foundInFocused = false;
        for (u64 fid : focusedIds)
        {
            if (mem.sourceStimulusId == fid)
            {
                foundInFocused = true;
                break;
            }
        }
        ASSERT_TRUE(foundInFocused);
    }

    return true;
}

// ============================================================
// C-8: Attention filtering
// ============================================================
TEST(cognitive_attention_bottleneck)
{
    WorldState world(32, 32, 42);
    world.Init(g_rulePack);

    world.Fields().WriteNext(g_envCtx.fire, 16, 16, 100.0f);
    world.Fields().WriteNext(g_envCtx.fire, 17, 16, 80.0f);
    world.Fields().WriteNext(g_envCtx.fire, 15, 16, 80.0f);
    world.Fields().SwapAll();

    // Strong smell
    for (i32 dy = -3; dy <= 3; dy++)
        for (i32 dx = -3; dx <= 3; dx++)
            if (world.Fields().InBounds(g_envCtx.smell, 16+dx, 15+dy))
                world.Fields().WriteNext(g_envCtx.smell, 16+dx, 15+dy, 50.0f);
    world.Fields().SwapAll();

    // Strong smoke
    for (i32 dy = -3; dy <= 3; dy++)
        for (i32 dx = -3; dx <= 3; dx++)
            if (world.Fields().InBounds(g_envCtx.smoke, 16+dx, 15+dy))
                world.Fields().WriteNext(g_envCtx.smoke, 16+dx, 15+dy, 40.0f);
    world.Fields().SwapAll();

    // Hot temperature
    for (i32 dy = -2; dy <= 2; dy++)
        for (i32 dx = -2; dx <= 2; dx++)
            if (world.Fields().InBounds(g_envCtx.temperature, 16+dx, 15+dy))
                world.Fields().WriteNext(g_envCtx.temperature, 16+dx, 15+dy, 45.0f);
    world.Fields().SwapAll();

    world.SpawnAgent(16, 15);
    world.RebuildSpatial();

    auto scheduler = CreateCognitiveScheduler(g_envCtx);
    scheduler.Tick(world);

    auto& cog = world.Cognitive();

    u32 stimulusCount = 0;
    for (const auto& s : cog.frameStimuli)
    {
        if (s.observerId == 1) stimulusCount++;
    }

    u32 focusedCount = 0;
    for (const auto& f : cog.frameFocused)
    {
        if (f.stimulus.observerId == 1) focusedCount++;
    }

    u32 memCount = 0;
    for (const auto& m : cog.frameMemories)
    {
        if (m.ownerId == 1) memCount++;
    }

    ASSERT_TRUE(focusedCount <= 4);
    ASSERT_TRUE(memCount <= focusedCount);

    if (stimulusCount > 4)
    {
        ASSERT_TRUE(focusedCount <= 4);
    }

    return true;
}

// ============================================================
// H-1: Different knowledge → different DecisionModifiers
// ============================================================
TEST(cognitive_knowledge_drives_behavior_divergence)
{
    // === Agent A with Fire→Danger knowledge ===
    WorldState worldA(32, 32, 42);
    worldA.Init(g_rulePack);

    {
        auto& kgA = worldA.Cognitive().knowledgeGraph;
        u64 fireId = kgA.GetOrCreateNode(1, 0, ConceptTag::Fire, 0).id;
        u64 painId = kgA.GetOrCreateNode(1, 0, ConceptTag::Pain, 0).id;
        auto& edgeA = kgA.GetOrCreateEdge(fireId, painId,
                                           KnowledgeRelation::Causes, 0);
        edgeA.confidence = 0.8f;
        edgeA.strength = 3.0f;
        edgeA.evidenceCount = 5;
    }

    worldA.SpawnAgent(16, 15);
    worldA.RebuildSpatial();

    auto modsA = worldA.Cognitive().GenerateDecisionModifiers(1);

    bool aHasFleeBoost = false;
    f32 aFleeMagnitude = 0.0f;
    for (const auto& m : modsA)
    {
        if (m.type == ModifierType::FleeBoost &&
            m.triggerConcept == ConceptTag::Fire)
        {
            aHasFleeBoost = true;
            aFleeMagnitude = m.magnitude;
            break;
        }
    }
    ASSERT_TRUE(aHasFleeBoost);
    ASSERT_TRUE(aFleeMagnitude > 0.0f);

    // === Agent B with Food→Satiety knowledge ===
    WorldState worldB(32, 32, 42);
    worldB.Init(g_rulePack);

    {
        auto& kgB = worldB.Cognitive().knowledgeGraph;
        u64 foodId = kgB.GetOrCreateNode(1, 0, ConceptTag::Food, 0).id;
        u64 satietyId = kgB.GetOrCreateNode(1, 0, ConceptTag::Satiety, 0).id;
        auto& edgeB = kgB.GetOrCreateEdge(foodId, satietyId,
                                           KnowledgeRelation::Causes, 0);
        edgeB.confidence = 0.7f;
        edgeB.strength = 2.0f;
        edgeB.evidenceCount = 4;
    }

    worldB.SpawnAgent(16, 15);
    worldB.RebuildSpatial();

    auto modsB = worldB.Cognitive().GenerateDecisionModifiers(1);

    bool bHasApproachBoost = false;
    f32 bApproachMagnitude = 0.0f;
    for (const auto& m : modsB)
    {
        if (m.type == ModifierType::ApproachBoost &&
            m.triggerConcept == ConceptTag::Food)
        {
            bHasApproachBoost = true;
            bApproachMagnitude = m.magnitude;
            break;
        }
    }
    ASSERT_TRUE(bHasApproachBoost);
    ASSERT_TRUE(bApproachMagnitude > 0.0f);

    // Agent A should NOT have ApproachBoost for Food
    bool aHasApproachForFood = false;
    for (const auto& m : modsA)
    {
        if (m.type == ModifierType::ApproachBoost &&
            m.triggerConcept == ConceptTag::Food)
        {
            aHasApproachForFood = true;
            break;
        }
    }
    ASSERT_TRUE(!aHasApproachForFood);

    // Agent B should NOT have FleeBoost for Fire
    bool bHasFleeForFire = false;
    for (const auto& m : modsB)
    {
        if (m.type == ModifierType::FleeBoost &&
            m.triggerConcept == ConceptTag::Fire)
        {
            bHasFleeForFire = true;
            break;
        }
    }
    ASSERT_TRUE(!bHasFleeForFire);

    ASSERT_TRUE(aFleeMagnitude != bApproachMagnitude);

    return true;
}

// ============================================================
// H-2: Same agent before/after learning
// ============================================================
TEST(cognitive_behavior_changes_with_learning)
{
    WorldState worldBefore(32, 32, 42);
    worldBefore.SpawnAgent(16, 15);
    worldBefore.RebuildSpatial();

    auto modsBefore = worldBefore.Cognitive().GenerateDecisionModifiers(1);
    ASSERT_TRUE(modsBefore.empty());

    WorldState worldAfter(32, 32, 42);

    {
        auto& kg = worldAfter.Cognitive().knowledgeGraph;
        u64 fireId = kg.GetOrCreateNode(1, 0, ConceptTag::Fire, 0).id;
        u64 dangerId = kg.GetOrCreateNode(1, 0, ConceptTag::Danger, 0).id;
        auto& edge = kg.GetOrCreateEdge(fireId, dangerId,
                                         KnowledgeRelation::Signals, 0);
        edge.confidence = 0.9f;
        edge.strength = 4.0f;
        edge.evidenceCount = 8;
    }

    worldAfter.SpawnAgent(16, 15);
    worldAfter.RebuildSpatial();

    auto modsAfter = worldAfter.Cognitive().GenerateDecisionModifiers(1);
    ASSERT_TRUE(modsAfter.size() > 0);

    bool hasFleeBoost = false;
    for (const auto& m : modsAfter)
    {
        if (m.type == ModifierType::FleeBoost &&
            m.triggerConcept == ConceptTag::Fire)
        {
            hasFleeBoost = true;
            ASSERT_TRUE(m.magnitude > 0.0f);
            ASSERT_TRUE(m.confidence == 0.9f);
            break;
        }
    }
    ASSERT_TRUE(hasFleeBoost);

    bool hasAlertBoost = false;
    for (const auto& m : modsAfter)
    {
        if (m.type == ModifierType::AlertBoost &&
            m.triggerConcept == ConceptTag::Fire)
        {
            hasAlertBoost = true;
            break;
        }
    }
    ASSERT_TRUE(hasAlertBoost);

    return true;
}

// ============================================================
// H-3: Natural learning end-to-end
// ============================================================
TEST(cognitive_natural_learning_changes_behavior)
{
    // Phase 1: No experience
    {
        WorldState world(32, 32, 42);
        world.Init(g_rulePack);
        world.SpawnAgent(16, 15);
        world.RebuildSpatial();

        auto nodes = world.Cognitive().knowledgeGraph.FindAgentNodes(1);
        ASSERT_TRUE(nodes.empty());
    }

    // Phase 2: Agent experiences fire environment
    {
        WorldState world(32, 32, 42);
        world.Init(g_rulePack);

        world.Fields().WriteNext(g_envCtx.fire, 16, 16, 100.0f);
        world.Fields().SwapAll();

        world.SpawnAgent(16, 15);
        world.RebuildSpatial();

        auto scheduler = CreateCognitiveScheduler(g_envCtx);

        for (i32 i = 0; i < 200; i++)
        {
            if (i % 10 == 0)
            {
                world.Fields().WriteNext(g_envCtx.fire, 16, 16, 100.0f);
                world.Fields().SwapAll();
            }
            scheduler.Tick(world);
        }

        auto& cog = world.Cognitive();

        auto& mems = cog.GetAgentMemories(1);
        ASSERT_TRUE(mems.size() > 0);

        auto& hyps = cog.GetAgentHypotheses(1);
        ASSERT_TRUE(hyps.size() > 0);

        bool hasFireHyp = false;
        for (const auto& h : hyps)
        {
            if (h.causeConcept == ConceptTag::Fire || h.effectConcept == ConceptTag::Fire)
            {
                hasFireHyp = true;
                break;
            }
        }
        ASSERT_TRUE(hasFireHyp);

        auto nodes = cog.knowledgeGraph.FindAgentNodes(1);
        ASSERT_TRUE(nodes.size() > 0);

        bool hasEdges = false;
        for (const auto* n : nodes)
        {
            auto edges = cog.knowledgeGraph.FindEdgesFrom(1, 0, n->concept);
            if (!edges.empty()) { hasEdges = true; break; }
        }
        ASSERT_TRUE(hasEdges);
    }

    return true;
}

// ============================================================
// H-4: Contradicted knowledge persists
// ============================================================
TEST(cognitive_contradicted_knowledge_persists)
{
    WorldState world(32, 32, 42);
    world.Init(g_rulePack);

    auto& cog = world.Cognitive();

    Hypothesis hyp;
    hyp.id = 1;
    hyp.ownerId = 1;
    hyp.causeConcept = ConceptTag::Fire;
    hyp.effectConcept = ConceptTag::Danger;
    hyp.proposedRelation = KnowledgeRelation::Signals;
    hyp.confidence = 0.15f;
    hyp.supportingCount = 2;
    hyp.contradictingCount = 3;
    hyp.status = HypothesisStatus::Contradicted;
    cog.GetAgentHypotheses(1).push_back(hyp);

    {
        auto& kg = cog.knowledgeGraph;
        u64 fireId = kg.GetOrCreateNode(1, 0, ConceptTag::Fire, 0).id;
        u64 dangerId = kg.GetOrCreateNode(1, 0, ConceptTag::Danger, 0).id;
        auto& edge = kg.GetOrCreateEdge(fireId, dangerId,
                                         KnowledgeRelation::Signals, 0);
        edge.confidence = 0.15f;
        edge.strength = 1.0f;
        edge.evidenceCount = 2;
    }

    world.SpawnAgent(16, 15);
    world.RebuildSpatial();

    auto mods = cog.GenerateDecisionModifiers(1);

    bool hasFleeBoost = false;
    for (const auto& m : mods)
    {
        if (m.type == ModifierType::FleeBoost &&
            m.triggerConcept == ConceptTag::Fire)
        {
            hasFleeBoost = true;
            ASSERT_TRUE(m.magnitude > 0.0f);
            ASSERT_TRUE(m.magnitude < 0.5f);
            break;
        }
    }
    ASSERT_TRUE(hasFleeBoost);

    return true;
}
