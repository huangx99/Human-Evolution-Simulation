#pragma once

// CognitiveModule: central storage for all cognitive data.
//
// === DATA LIFECYCLE ===
//
// Frame-level transient data (cleared each tick via ClearFrame()):
//   - frameStimuli: PerceivedStimulus from CognitivePerceptionSystem
//   - frameFocused: FocusedStimulus from CognitiveAttentionSystem
//   - frameMemories: MemoryRecord from CognitiveMemorySystem
//   - frameDiscoveries: DiscoveryRecord from CognitiveDiscoverySystem
//
// Agent-lifetime persistent data (accumulates over simulation):
//   - agentMemories: per-agent memory records (decay over time, never cleared)
//   - agentHypotheses: per-agent hypothesis states
//   - knowledgeGraph: shared graph with per-agent nodes/edges
//
// RULE: Downstream systems consume frame buffers, NOT raw WorldState.
// CognitivePerceptionSystem is the ONLY system that reads WorldState
// to produce PerceivedStimulus. Everything else reads from frame buffers
// or persistent cognitive data.

#include "sim/world/module_registry.h"
#include "sim/cognitive/perceived_stimulus.h"
#include "sim/cognitive/focused_stimulus.h"
#include "sim/cognitive/memory_record.h"
#include "sim/cognitive/hypothesis.h"
#include "sim/cognitive/knowledge_graph.h"
#include "sim/cognitive/decision_modifier.h"
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>

struct CognitiveModule : public IModule
{
    // === Per-frame buffers (cleared each tick) ===

    std::vector<PerceivedStimulus> frameStimuli;
    std::vector<FocusedStimulus> frameFocused;
    std::vector<MemoryRecord> frameMemories;
    std::vector<DiscoveryRecord> frameDiscoveries;

    // === Per-agent cognitive summary (populated by CognitivePerceptionSystem) ===
    //
    // ARCHITECTURE NOTE: This is the bridge between cognitive perception and
    // runtime decision-making. DecisionSystem reads from here, NOT from raw
    // world fields. This maintains the boundary: only CognitivePerceptionSystem
    // reads WorldState; all downstream systems use cognitive data.

    // === Persistent per-agent data ===

    std::unordered_map<EntityId, std::vector<MemoryRecord>> agentMemories;
    std::unordered_map<EntityId, std::vector<Hypothesis>> agentHypotheses;

    // === Global knowledge graph (shared structure, per-agent nodes) ===

    KnowledgeGraph knowledgeGraph;

    // === ID generators ===

    u64 nextStimulusId = 1;
    u64 nextMemoryId = 1;
    u64 nextHypothesisId = 1;
    u64 nextDiscoveryId = 1;

    // === Per-frame lifecycle ===

    void ClearFrame()
    {
        frameStimuli.clear();
        frameFocused.clear();
        frameMemories.clear();
        frameDiscoveries.clear();
    }

    // === Memory helpers ===

    std::vector<MemoryRecord>& GetAgentMemories(EntityId agentId)
    {
        return agentMemories[agentId];
    }

    const std::vector<MemoryRecord>& GetAgentMemories(EntityId agentId) const
    {
        auto it = agentMemories.find(agentId);
        static const std::vector<MemoryRecord> empty;
        return it != agentMemories.end() ? it->second : empty;
    }

    // === Hypothesis helpers ===

    std::vector<Hypothesis>& GetAgentHypotheses(EntityId agentId)
    {
        return agentHypotheses[agentId];
    }

    // === Memory management ===

    void DecayAllMemories(f32 shortTermRate, f32 longTermRate)
    {
        for (auto& [agentId, memories] : agentMemories)
        {
            for (auto& mem : memories)
            {
                f32 rate = (mem.kind == MemoryKind::ShortTerm)
                    ? shortTermRate : longTermRate;
                mem.Decay(rate);
            }
            memories.erase(
                std::remove_if(memories.begin(), memories.end(),
                    [](const MemoryRecord& m) { return m.IsFaded(); }),
                memories.end());
        }
    }

    // === Legacy memory promotion helper: retained for callers that explicitly opt in ===

    void ConsolidateMemories(f32 promotionThreshold)
    {
        for (auto& [agentId, memories] : agentMemories)
        {
            for (auto& mem : memories)
            {
                if (mem.kind == MemoryKind::ShortTerm &&
                    mem.strength >= promotionThreshold)
                {
                    mem.kind = MemoryKind::Episodic;
                }
            }
        }
    }

    // === Knowledge feedback: query what an agent knows about a concept ===
    //
    // Returns a danger score (0..1+) based on knowledge edges.
    // Used by Attention and Decision systems to bias behavior.
    //
    // Example: if agent knows Fire→Causes→Pain, returns high danger.
    //          if agent knows Food→Causes→Satiety, returns low danger.

    f32 GetKnownDanger(EntityId agentId, ConceptTypeId concept) const
    {
        const auto& reg = ConceptTypeRegistry::Instance();
        f32 danger = 0.0f;
        for (const auto& e : knowledgeGraph.edges)
        {
            const auto* fromNode = knowledgeGraph.FindNodeById(e.fromNodeId);
            if (!fromNode || fromNode->ownerAgentId != agentId) continue;
            if (fromNode->concept != concept) continue;

            if (e.relation == KnowledgeRelation::Causes ||
                e.relation == KnowledgeRelation::Signals)
            {
                const auto* toNode = knowledgeGraph.FindNodeById(e.toNodeId);
                if (toNode)
                {
                    if (reg.HasFlag(toNode->concept, ConceptSemanticFlag::Danger) ||
                        reg.HasFlag(toNode->concept, ConceptSemanticFlag::Threat))
                    {
                        danger += e.confidence * e.strength;
                    }
                }
            }
        }
        return danger;
    }

    // Check if an agent has knowledge that a concept signals/causes another
    bool HasKnowledgeLink(EntityId agentId, ConceptTypeId from,
                          ConceptTypeId to, KnowledgeRelation relation) const
    {
        for (const auto& e : knowledgeGraph.edges)
        {
            const auto* fromNode = knowledgeGraph.FindNodeById(e.fromNodeId);
            if (!fromNode || fromNode->ownerAgentId != agentId) continue;
            if (fromNode->concept != from) continue;
            if (e.relation != relation) continue;

            const auto* toNode = knowledgeGraph.FindNodeById(e.toNodeId);
            if (toNode && toNode->concept == to)
                return true;
        }
        return false;
    }

    // === Debug: dump knowledge graph for an agent ===

    std::string DumpKnowledge(EntityId agentId) const
    {
        return knowledgeGraph.Dump(agentId);
    }

    // === Decision modifiers: knowledge-driven behavior bias ===
    //
    // Queries the knowledge graph and produces generic modifiers.
    // DecisionSystem uses these instead of hand-written knowledge checks.
    // Adding new knowledge types doesn't require touching DecisionSystem.

    std::vector<DecisionModifier> GenerateDecisionModifiers(EntityId agentId) const
    {
        std::vector<DecisionModifier> mods;
        const auto& reg = ConceptTypeRegistry::Instance();

        for (const auto& e : knowledgeGraph.edges)
        {
            const auto* fromNode = knowledgeGraph.FindNodeById(e.fromNodeId);
            if (!fromNode || fromNode->ownerAgentId != agentId) continue;

            const auto* toNode = knowledgeGraph.FindNodeById(e.toNodeId);
            if (!toNode) continue;

            // Skip weak edges
            if (e.confidence < 0.1f) continue;

            // Knowledge that A causes/signals something dangerous → flee boost for A
            if ((e.relation == KnowledgeRelation::Causes ||
                 e.relation == KnowledgeRelation::Signals) &&
                (reg.HasFlag(toNode->concept, ConceptSemanticFlag::Danger) ||
                 reg.HasFlag(toNode->concept, ConceptSemanticFlag::Threat)))
            {
                DecisionModifier mod;
                mod.type = ModifierType::FleeBoost;
                mod.triggerConcept = fromNode->concept;
                mod.magnitude = e.confidence * e.strength * 0.3f;
                mod.confidence = e.confidence;
                mods.push_back(mod);
            }

            // Knowledge that A causes something valued → approach boost for A
            if (e.relation == KnowledgeRelation::Causes &&
                (reg.HasFlag(toNode->concept, ConceptSemanticFlag::Positive)))
            {
                DecisionModifier mod;
                mod.type = ModifierType::ApproachBoost;
                mod.triggerConcept = fromNode->concept;
                mod.magnitude = e.confidence * e.strength * 0.3f;
                mod.confidence = e.confidence;
                mods.push_back(mod);
            }

            // Knowledge that A signals B → alert boost for A
            if (e.relation == KnowledgeRelation::Signals)
            {
                DecisionModifier mod;
                mod.type = ModifierType::AlertBoost;
                mod.triggerConcept = fromNode->concept;
                mod.magnitude = e.confidence * 0.2f;
                mod.confidence = e.confidence;
                mods.push_back(mod);
            }
        }

        return mods;
    }
};
