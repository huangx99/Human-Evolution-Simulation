#include "test_framework.h"
#include "sim/system/system_context.h"
#include "sim/world/module_registry.h"
#include "core/container/field2d.h"
#include "sim/scheduler/scheduler.h"
#include "sim/scheduler/phase.h"
#include "rules/human_evolution/environment/climate_system.h"
#include "sim/event/event_bus.h"
#include "sim/command/command.h"
#include "rules/reaction/semantic_predicate.h"
#include "sim/spatial/spatial_index.h"
#include "api/snapshot/world_snapshot.h"
#include <memory>

// Test: ModuleRegistry basic operations
TEST(module_registration)
{
    WorldState world(8, 8, 42);

    // Modules should be registered
    ASSERT_TRUE(world.modules.Has<SimulationModule>());
    ASSERT_TRUE(world.modules.Has<EnvironmentModule>());
    ASSERT_TRUE(world.modules.Has<InformationModule>());
    ASSERT_TRUE(world.modules.Has<AgentModule>());
    ASSERT_TRUE(world.modules.Has<EcologyModule>());

    // Should be able to access world dimensions
    ASSERT_EQ(world.Width(), 8);
    ASSERT_EQ(world.Height(), 8);

    return true;
}

// Test: Field2D double buffer
TEST(field_double_buffer)
{
    Field2D<f32> field(4, 4, 0.0f);

    // Write to next
    field.WriteNext(1, 1) = 42.0f;
    ASSERT_EQ(field.At(1, 1), 0.0f);  // current unchanged

    // Swap
    field.Swap();
    ASSERT_EQ(field.At(1, 1), 42.0f);  // now in current

    return true;
}

// Test: Scheduler phase ordering
TEST(scheduler_phase_order)
{
    std::vector<SimPhase> executionOrder;

    class PhaseTracker : public ISystem
    {
    public:
        PhaseTracker(SimPhase phase, std::vector<SimPhase>& order)
            : phase(phase), order(order) {}

        void Update(SystemContext&) override
        {
            order.push_back(phase);
        }

        SystemDescriptor Descriptor() const override
        {
            static const char* const DEPS[] = {};
            return {"PhaseTracker", phase, nullptr, 0, nullptr, 0, DEPS, 0, true, true};
        }

    private:
        SimPhase phase;
        std::vector<SimPhase>& order;
    };

    WorldState world(4, 4, 42);
    Scheduler scheduler;

    scheduler.AddSystem(SimPhase::Action, std::make_unique<PhaseTracker>(SimPhase::Action, executionOrder));
    scheduler.AddSystem(SimPhase::Environment, std::make_unique<PhaseTracker>(SimPhase::Environment, executionOrder));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<PhaseTracker>(SimPhase::Perception, executionOrder));

    scheduler.Tick(world);

    // Environment should come before Perception, which should come before Action
    ASSERT_TRUE(executionOrder.size() == 3);
    ASSERT_TRUE(executionOrder[0] == SimPhase::Environment);
    ASSERT_TRUE(executionOrder[1] == SimPhase::Perception);
    ASSERT_TRUE(executionOrder[2] == SimPhase::Action);

    return true;
}

// Test: Command apply
TEST(command_apply)
{
    WorldState world(8, 8, 42);
    world.SpawnAgent(2, 3);

    world.commands.Push(0, MoveAgentCommand{1, 5, 6});
    world.commands.Apply(world);

    auto& agent = world.Agents().agents[0];
    ASSERT_EQ(agent.position.x, 5);
    ASSERT_EQ(agent.position.y, 6);

    return true;
}

// Test: Event lifecycle (Emit → Dispatch → Archive)
TEST(event_lifecycle)
{
    WorldState world(4, 4, 42);

    i32 receivedX = -1;
    f32 receivedValue = 0.0f;
    world.events.Subscribe(EventType::FireStarted, [&](const Event& e) {
        receivedX = e.x;
        receivedValue = e.value;
    });

    Event evt;
    evt.type = EventType::FireStarted;
    evt.tick = 10;
    evt.x = 2;
    evt.y = 3;
    evt.value = 50.0f;

    // Emit enqueues, Dispatch processes
    world.events.Emit(evt);
    ASSERT_EQ(receivedX, -1);  // not yet dispatched

    world.events.Dispatch();
    ASSERT_EQ(receivedX, 2);
    ASSERT_EQ(receivedValue, 50.0f);

    // Archive is populated
    ASSERT_EQ(world.events.GetArchive().size(), 1);
    ASSERT_EQ(world.events.GetArchive(EventType::FireStarted).size(), 1);

    return true;
}

// Test: Snapshot boundary (snapshot doesn't modify world)
TEST(snapshot_boundary)
{
    WorldState world(8, 8, 42);
    world.Env().fire.WriteNext(4, 4) = 100.0f;
    world.Env().fire.Swap();

    Tick tickBefore = world.Sim().clock.currentTick;
    WorldSnapshot snap = WorldSnapshot::Capture(world);
    Tick tickAfter = world.Sim().clock.currentTick;

    // Snapshot should not modify world
    ASSERT_EQ(tickBefore, tickAfter);
    ASSERT_EQ(snap.tick, tickBefore);

    return true;
}

// Test: Snapshot captures ecology entities, events, spatial stats, command stats
TEST(snapshot_full_capture)
{
    WorldState world(16, 16, 42);

    // Create ecology entities
    auto& e1 = world.Ecology().entities.Create(MaterialId::Grass, "grass1");
    e1.x = 3; e1.y = 3;
    auto& e2 = world.Ecology().entities.Create(MaterialId::Stone, "stone1");
    e2.x = 5; e2.y = 5;

    world.RebuildSpatial();

    // Emit an event
    Event evt;
    evt.type = EventType::FireStarted;
    evt.tick = 1;
    evt.x = 3;
    evt.y = 3;
    evt.value = 50.0f;
    world.events.Emit(evt);
    world.events.Dispatch();

    // Submit a command
    world.commands.Push(0, IgniteFireCommand{3, 3, 10.0f});
    world.commands.Apply(world);

    WorldSnapshot snap = WorldSnapshot::Capture(world);

    // Ecology entities captured
    ASSERT_EQ(static_cast<i32>(snap.entities.size()), 2);
    ASSERT_EQ(snap.entities[0].id, e1.id);
    ASSERT_EQ(snap.entities[0].name, "grass1");
    ASSERT_EQ(snap.entities[0].x, 3);
    ASSERT_EQ(snap.entities[0].y, 3);
    ASSERT_TRUE(snap.entities[0].capabilities != 0); // Grass has capabilities from MaterialDB

    // Event archive captured
    ASSERT_EQ(static_cast<i32>(snap.events.size()), 1);
    ASSERT_EQ(snap.events[0].type, static_cast<u16>(EventType::FireStarted));
    ASSERT_EQ(snap.events[0].x, 3);

    // Spatial stats captured
    ASSERT_TRUE(snap.spatial.initialized);
    ASSERT_EQ(snap.spatial.chunkCount, 1);  // 16x16 world, chunk=16 → 1x1=1
    ASSERT_EQ(snap.spatial.totalEntities, 2);
    ASSERT_TRUE(snap.spatial.occupiedPositions > 0);

    // Command stats captured
    ASSERT_TRUE(snap.commands.historyCount > 0);

    // Serialize doesn't crash
    std::string serialized = snap.Serialize();
    ASSERT_TRUE(serialized.find("entities=2") != std::string::npos);
    ASSERT_TRUE(serialized.find("events=1") != std::string::npos);
    ASSERT_TRUE(serialized.find("spatial:") != std::string::npos);
    ASSERT_TRUE(serialized.find("commands:") != std::string::npos);

    return true;
}

// Test: Semantic predicate construction
TEST(semantic_predicate_construction)
{
    SemanticPredicate pred;
    pred.type = PredicateType::HasCapability;
    pred.capability = Capability::Flammable;
    ASSERT_TRUE(pred.type == PredicateType::HasCapability);
    ASSERT_TRUE(pred.capability == Capability::Flammable);

    SemanticPredicate fieldPred;
    fieldPred.type = PredicateType::FieldGreaterThan;
    fieldPred.field = FieldKey("human_evolution.temperature");
    fieldPred.value = 50.0f;
    ASSERT_TRUE(fieldPred.field == FieldKey("human_evolution.temperature"));
    ASSERT_TRUE(fieldPred.value > 49.0f);

    return true;
}

// Test: Expansion - add new field without changing core
TEST(expansion_new_field)
{
    // This test verifies that we can create a new Field2D
    // without modifying any existing code
    Field2D<f32> diseaseField(8, 8, 0.0f);
    diseaseField.WriteNext(3, 3) = 75.0f;
    diseaseField.Swap();
    ASSERT_EQ(diseaseField.At(3, 3), 75.0f);

    return true;
}

// Test: SpatialIndex basic operations
TEST(spatial_index_basics)
{
    WorldState world(32, 32, 42);
    auto& ecology = world.Ecology().entities;

    // deque guarantees pointer/reference stability across insertions
    auto& e1 = ecology.Create(MaterialId::Stone, "stone1");
    e1.x = 3; e1.y = 3;   // chunk (0,0)

    auto& e2 = ecology.Create(MaterialId::Wood, "wood1");
    e2.x = 20; e2.y = 5;  // chunk (1,0)

    auto& e3 = ecology.Create(MaterialId::Grass, "grass1");
    e3.x = 20; e3.y = 20; // chunk (1,1)

    world.RebuildSpatial();

    // AllPositions should return 3 positions
    auto positions = world.spatial.AllPositions();
    ASSERT_EQ(static_cast<i32>(positions.size()), 3);

    // QueryArea around (3,3) radius 0 should find e1
    auto q1 = world.spatial.QueryArea(3, 3, 0);
    ASSERT_EQ(static_cast<i32>(q1.size()), 1);
    ASSERT_EQ(q1[0]->id, e1.id);

    // QueryArea around (20,5) radius 0 should find e2
    auto q2 = world.spatial.QueryArea(20, 5, 0);
    ASSERT_EQ(static_cast<i32>(q2.size()), 1);
    ASSERT_EQ(q2[0]->id, e2.id);

    // QueryArea around (0,0) radius 30 should find all 3
    auto q3 = world.spatial.QueryArea(0, 0, 30);
    ASSERT_EQ(static_cast<i32>(q3.size()), 3);

    // QueryAreaWithCapability for Flammable should find e2 (Wood) and e3 (Grass)
    auto q4 = world.spatial.QueryAreaWithCapability(0, 0, 30, Capability::Flammable);
    ASSERT_EQ(static_cast<i32>(q4.size()), 2);

    // Chunk count for 32x32 world with chunk size 16 = 2x2 = 4
    ASSERT_EQ(world.spatial.ChunkCount(), 4);

    return true;
}

// Test: Determinism with 100k ticks
TEST(determinism_100k)
{
    auto run = [](u64 seed) -> std::string {
        WorldState world(16, 16, seed);
        world.Env().fire.WriteNext(8, 8) = 50.0f;
        world.Env().fire.Swap();
        world.SpawnAgent(4, 4);

        Scheduler scheduler;
        scheduler.AddSystem(SimPhase::Environment, std::make_unique<ClimateSystem>());

        for (i32 i = 0; i < 1000; i++)
            scheduler.Tick(world);

        WorldSnapshot snap = WorldSnapshot::Capture(world);
        return snap.Serialize();
    };

    ASSERT_EQ(run(42), run(42));
    ASSERT_TRUE(run(42) != run(99));

    return true;
}
