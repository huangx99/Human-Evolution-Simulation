#pragma once

// KnowledgeGraph: an agent's web of beliefs about how concepts relate.
//
// ARCHITECTURE NOTE: KnowledgeGraph is NOT a tech tree, NOT a bool unlock,
// and NOT objective truth. It represents an agent's subjective beliefs about
// concept relationships. These beliefs can be wrong, incomplete, or
// contradictory between agents.
//
// Structure:
//   - KnowledgeNode: a concept the agent "knows about" (e.g., Fire, Food)
//   - KnowledgeEdge: a believed relation between two concepts
//     (e.g., Fire →Causes→ Pain, Smoke →Signals→ Fire)
//
// Edges have confidence (0..1) and evidence count. They grow stronger
// through repeated observation and weaker through contradiction.
//
// The graph drives behavior: attention bias, decision bias, and social
// learning all query the graph. This closes the loop:
//   Perception → Attention → Memory → Hypothesis → Knowledge → Behavior

#include "core/types/types.h"
#include "sim/cognitive/concept_tag.h"
#include "sim/cognitive/knowledge_relation.h"
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>

struct KnowledgeNode
{
    u64 id = 0;
    EntityId ownerAgentId = 0;  // 0 = group-owned
    u64 groupId = 0;            // 0 = individual

    ConceptTag concept = ConceptTag::None;

    f32 strength = 0.0f;       // how strongly this concept is "known"
    Tick firstKnownTick = 0;
    Tick lastUpdatedTick = 0;
};

struct KnowledgeEdge
{
    u64 id = 0;

    u64 fromNodeId = 0;
    u64 toNodeId = 0;

    KnowledgeRelation relation = KnowledgeRelation::AssociatedWith;

    f32 confidence = 0.0f;     // 0..1, how certain this relation is
    f32 strength = 0.0f;       // how reinforced this edge is
    u32 evidenceCount = 0;     // how many observations support this

    Tick firstObservedTick = 0;
    Tick lastObservedTick = 0;
};

struct KnowledgeGraph
{
    std::vector<KnowledgeNode> nodes;
    std::vector<KnowledgeEdge> edges;

    u64 nextNodeId = 1;
    u64 nextEdgeId = 1;

    // --- Node operations ---

    KnowledgeNode* FindNode(EntityId owner, u64 groupId, ConceptTag concept)
    {
        for (auto& n : nodes)
        {
            if (n.ownerAgentId == owner &&
                n.groupId == groupId &&
                n.concept == concept)
                return &n;
        }
        return nullptr;
    }

    KnowledgeNode& GetOrCreateNode(EntityId owner, u64 groupId,
                                    ConceptTag concept, Tick tick)
    {
        auto* existing = FindNode(owner, groupId, concept);
        if (existing) return *existing;

        KnowledgeNode node;
        node.id = nextNodeId++;
        node.ownerAgentId = owner;
        node.groupId = groupId;
        node.concept = concept;
        node.firstKnownTick = tick;
        node.lastUpdatedTick = tick;
        nodes.push_back(node);
        return nodes.back();
    }

    // --- Edge operations ---

    KnowledgeEdge* FindEdge(u64 from, u64 to, KnowledgeRelation relation)
    {
        for (auto& e : edges)
        {
            if (e.fromNodeId == from &&
                e.toNodeId == to &&
                e.relation == relation)
                return &e;
        }
        return nullptr;
    }

    KnowledgeEdge& GetOrCreateEdge(u64 from, u64 to,
                                    KnowledgeRelation relation, Tick tick)
    {
        auto* existing = FindEdge(from, to, relation);
        if (existing) return *existing;

        KnowledgeEdge edge;
        edge.id = nextEdgeId++;
        edge.fromNodeId = from;
        edge.toNodeId = to;
        edge.relation = relation;
        edge.firstObservedTick = tick;
        edge.lastObservedTick = tick;
        edges.push_back(edge);
        return edges.back();
    }

    void ReinforceEdge(KnowledgeEdge& edge, f32 confidenceDelta, Tick tick)
    {
        edge.confidence = std::min(1.0f, edge.confidence + confidenceDelta);
        edge.strength += 1.0f;
        edge.evidenceCount++;
        edge.lastObservedTick = tick;
    }

    // --- Query: outgoing edges from a concept ---

    std::vector<KnowledgeEdge*> FindEdgesFrom(EntityId owner, u64 groupId,
                                               ConceptTag concept)
    {
        auto* node = FindNode(owner, groupId, concept);
        if (!node) return {};

        std::vector<KnowledgeEdge*> result;
        for (auto& e : edges)
        {
            if (e.fromNodeId == node->id)
                result.push_back(&e);
        }
        return result;
    }

    // --- Query: incoming edges to a concept ---

    std::vector<KnowledgeEdge*> FindEdgesTo(EntityId owner, u64 groupId,
                                             ConceptTag concept)
    {
        auto* node = FindNode(owner, groupId, concept);
        if (!node) return {};

        std::vector<KnowledgeEdge*> result;
        for (auto& e : edges)
        {
            if (e.toNodeId == node->id)
                result.push_back(&e);
        }
        return result;
    }

    // --- Query: all edges involving a concept (in or out) ---

    std::vector<KnowledgeEdge*> FindAllEdges(EntityId owner, u64 groupId,
                                              ConceptTag concept)
    {
        auto* node = FindNode(owner, groupId, concept);
        if (!node) return {};

        std::vector<KnowledgeEdge*> result;
        for (auto& e : edges)
        {
            if (e.fromNodeId == node->id || e.toNodeId == node->id)
                result.push_back(&e);
        }
        return result;
    }

    // --- Query: edges by relation type ---

    std::vector<KnowledgeEdge*> FindEdgesByRelation(EntityId owner, u64 groupId,
                                                     KnowledgeRelation relation)
    {
        std::vector<KnowledgeEdge*> result;
        for (auto& e : edges)
        {
            if (e.relation == relation)
            {
                auto* fromNode = FindNodeById(e.fromNodeId);
                if (fromNode && fromNode->ownerAgentId == owner &&
                    fromNode->groupId == groupId)
                    result.push_back(&e);
            }
        }
        return result;
    }

    // --- Query: find connected concepts (neighbors) ---

    std::vector<ConceptTag> FindNeighbors(EntityId owner, u64 groupId,
                                           ConceptTag concept)
    {
        auto* node = FindNode(owner, groupId, concept);
        if (!node) return {};

        std::vector<ConceptTag> result;
        for (auto& e : edges)
        {
            if (e.fromNodeId == node->id)
            {
                auto* toNode = FindNodeById(e.toNodeId);
                if (toNode) result.push_back(toNode->concept);
            }
            else if (e.toNodeId == node->id)
            {
                auto* fromNode = FindNodeById(e.fromNodeId);
                if (fromNode) result.push_back(fromNode->concept);
            }
        }
        return result;
    }

    // --- Query: all nodes for an agent ---

    std::vector<KnowledgeNode*> FindAgentNodes(EntityId owner)
    {
        std::vector<KnowledgeNode*> result;
        for (auto& n : nodes)
        {
            if (n.ownerAgentId == owner)
                result.push_back(&n);
        }
        return result;
    }

    // --- Stats ---

    size_t NodeCount() const { return nodes.size(); }
    size_t EdgeCount() const { return edges.size(); }

    // --- Node lookup by ID (needed for edge traversal) ---

    KnowledgeNode* FindNodeById(u64 id)
    {
        for (auto& n : nodes)
        {
            if (n.id == id) return &n;
        }
        return nullptr;
    }

    const KnowledgeNode* FindNodeById(u64 id) const
    {
        for (const auto& n : nodes)
        {
            if (n.id == id) return &n;
        }
        return nullptr;
    }

    // --- Debug dump ---

    std::string Dump(EntityId agentId) const
    {
        std::ostringstream os;
        for (const auto& e : edges)
        {
            const auto* fromNode = FindNodeById(e.fromNodeId);
            if (!fromNode || fromNode->ownerAgentId != agentId) continue;

            const auto* toNode = FindNodeById(e.toNodeId);
            if (!toNode) continue;

            os << ConceptRegistry::GetName(fromNode->concept) << " -> "
               << ConceptRegistry::GetName(toNode->concept)
               << " [" << RelationName(e.relation) << "]"
               << " confidence=" << std::fixed << std::setprecision(2) << e.confidence
               << " evidence=" << e.evidenceCount << "\n";
        }
        return os.str();
    }

private:
    static const char* RelationName(KnowledgeRelation r)
    {
        switch (r)
        {
        case KnowledgeRelation::Causes:         return "Causes";
        case KnowledgeRelation::Prevents:       return "Prevents";
        case KnowledgeRelation::Attracts:       return "Attracts";
        case KnowledgeRelation::Repels:         return "Repels";
        case KnowledgeRelation::Transforms:     return "Transforms";
        case KnowledgeRelation::Emits:          return "Emits";
        case KnowledgeRelation::Contains:       return "Contains";
        case KnowledgeRelation::Requires:       return "Requires";
        case KnowledgeRelation::Replaces:       return "Replaces";
        case KnowledgeRelation::Signals:        return "Signals";
        case KnowledgeRelation::FearedAs:       return "FearedAs";
        case KnowledgeRelation::ValuedAs:       return "ValuedAs";
        case KnowledgeRelation::SacredAs:       return "SacredAs";
        case KnowledgeRelation::AssociatedWith: return "AssociatedWith";
        default: return "?";
        }
    }
};
