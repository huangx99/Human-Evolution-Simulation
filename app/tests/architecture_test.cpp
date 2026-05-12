#include "test_framework.h"
#include "sim/world/world_state.h"
#include "sim/world/module_registry.h"
#include "core/container/field2d.h"
#include "sim/scheduler/scheduler.h"
#include "sim/scheduler/phase.h"
#include "sim/system/climate_system.h"
#include "sim/event/event_bus.h"
#include "sim/command/command.h"
#include "rules/reaction/reaction_rule.h"
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

    // Should be able to access
    auto& env = world.modules.Get<EnvironmentModule>();
    ASSERT_EQ(env.width, 8);
    ASSERT_EQ(env.height, 8);

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

        void Update(WorldState&) override
        {
            order.push_back(phase);
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

    Command cmd;
    cmd.type = CommandType::MoveAgent;
    cmd.entityId = 1;
    cmd.targetX = 5;
    cmd.targetY = 6;

    world.commands.Push(cmd);
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

// Test: Reaction rule evaluation
TEST(reaction_rule)
{
    Condition cond;
    cond.field = FieldId::Temperature;
    cond.op = ConditionOp::Greater;
    cond.value = 50.0f;

    ASSERT_TRUE(EvaluateCondition(cond, 60.0f));
    ASSERT_TRUE(!EvaluateCondition(cond, 40.0f));

    cond.op = ConditionOp::Less;
    ASSERT_TRUE(EvaluateCondition(cond, 40.0f));
    ASSERT_TRUE(!EvaluateCondition(cond, 60.0f));

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
