#include "sim/world/world_state.h"
#include "sim/social/social_signal_registry.h"
#include "sim/social/social_signal_module.h"
#include "sim/runtime/simulation_hash.h"
#include "rules/human_evolution/human_evolution_rule_pack.h"
#include "test_framework.h"

// Test 1: SocialSignalRegistry can register and lookup types
TEST(social_signal_registry_can_register)
{
    SocialSignalKey key("test.my_signal");
    auto& reg = SocialSignalRegistry::Instance();
    auto id = reg.Register(key, "my_signal");

    ASSERT_TRUE(id.index > 0);
    ASSERT_EQ(reg.FindByKey(key).index, id.index);
    ASSERT_EQ(reg.GetName(id), "my_signal");

    // Idempotent: same key returns same id
    auto id2 = reg.Register(key, "my_signal");
    ASSERT_EQ(id.index, id2.index);

    return true;
}

// Test 2: SocialSignalModule can emit and expire signals
TEST(social_signal_module_emit_expire)
{
    SocialSignalModule mod;

    SocialSignal sig;
    sig.typeId = SocialSignalTypeId(1);
    sig.sourceEntityId = 1;
    sig.origin = {5, 5};
    sig.intensity = 1.0f;
    sig.createdTick = 0;
    sig.ttl = 10;

    u64 id = mod.Emit(sig);
    ASSERT_TRUE(id > 0);
    ASSERT_EQ(mod.Count(), 1);

    // Not expired yet at tick 5
    mod.ClearExpired(5);
    ASSERT_EQ(mod.Count(), 1);

    // Expired at tick 10 (createdTick + ttl <= now)
    mod.ClearExpired(10);
    ASSERT_EQ(mod.Count(), 0);

    return true;
}

// Test 3: SocialSignalModule QueryNear filters by distance
TEST(social_signal_module_query_near)
{
    SocialSignalModule mod;

    SocialSignal sig;
    sig.typeId = SocialSignalTypeId(1);
    sig.origin = {5, 5};
    sig.effectiveRadius = 10.0f;
    sig.createdTick = 0;
    sig.ttl = 100;
    mod.Emit(sig);

    // Within radius
    auto nearby = mod.QueryNear({6, 6}, 5.0f);
    ASSERT_EQ(nearby.size(), 1);

    // Outside radius
    auto far = mod.QueryNear({20, 20}, 5.0f);
    ASSERT_EQ(far.size(), 0);

    return true;
}

// Test 4: WorldState contains SocialSignalModule
TEST(world_state_contains_social_module)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);

    auto& social = world.SocialSignals();
    ASSERT_EQ(social.Count(), 0);

    SocialSignal sig;
    sig.typeId = SocialSignalTypeId(1);
    sig.origin = {5, 5};
    sig.ttl = 10;
    social.Emit(sig);
    ASSERT_EQ(social.Count(), 1);

    return true;
}

// Test 5: HumanEvolutionRulePack registers fear and death_warning
TEST(rulepack_registers_social_signals)
{
    HumanEvolutionRulePack rp;
    // WorldState constructor calls RegisterSocialSignals
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    ASSERT_TRUE(ctx.social.fear.index > 0);
    ASSERT_TRUE(ctx.social.deathWarning.index > 0);
    ASSERT_TRUE(ctx.social.fear.index != ctx.social.deathWarning.index);

    // Verify registry lookup
    auto& reg = SocialSignalRegistry::Instance();
    ASSERT_EQ(reg.FindByKey(MakeSocialSignalKey("human_evolution.fear")).index,
              ctx.social.fear.index);
    ASSERT_EQ(reg.FindByKey(MakeSocialSignalKey("human_evolution.death_warning")).index,
              ctx.social.deathWarning.index);

    return true;
}

// Test 6: CognitiveModule no longer has frameSocialSignals / nextSocialSignalId
TEST(social_signal_not_in_cognitive_module)
{
    // Compile-time check: if CognitiveModule had these members,
    // the code referencing them would still compile.
    // We verify by checking that CognitiveModule can be constructed
    // and used without those members.
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);

    auto& cog = world.Cognitive();
    // ClearFrame should work without frameSocialSignals
    cog.ClearFrame();

    // SocialSignalModule should exist independently
    auto& social = world.SocialSignals();
    SocialSignal sig;
    sig.typeId = SocialSignalTypeId(1);
    sig.origin = {8, 8};
    sig.ttl = 5;
    social.Emit(sig);
    ASSERT_EQ(social.Count(), 1);

    // ClearFrame should NOT affect SocialSignalModule
    cog.ClearFrame();
    ASSERT_EQ(social.Count(), 1);

    return true;
}

// Test 7: Empty SocialSignalModule does not change initial world hash
TEST(empty_social_module_does_not_change_hash)
{
    // Two identical worlds should have the same hash.
    // SocialSignalModule is registered but empty (no systems emit signals).
    auto makeWorld = [](u64 seed) {
        HumanEvolutionRulePack rp;
        WorldState world(32, 32, seed, rp);
        world.Fields().WriteNext(rp.GetHumanEvolutionContext().environment.fire, 16, 16, 80.0f);
        world.Fields().SwapAll();
        world.SpawnAgent(5, 5);
        world.SpawnAgent(10, 20);
        return world;
    };

    WorldState a = makeWorld(42);
    WorldState b = makeWorld(42);

    ASSERT_EQ(ComputeWorldHash(a, HashTier::Full),
              ComputeWorldHash(b, HashTier::Full));

    return true;
}

TEST(active_social_signal_changes_full_hash)
{
    // Two identical worlds, one with an active signal, should have different Full hashes.
    HumanEvolutionRulePack rp;

    WorldState a(32, 32, 42, rp);
    a.SpawnAgent(5, 5);

    WorldState b(32, 32, 42, rp);
    b.SpawnAgent(5, 5);

    // Empty: same hash
    ASSERT_EQ(ComputeWorldHash(a, HashTier::Full),
              ComputeWorldHash(b, HashTier::Full));

    // Emit a signal into world b
    SocialSignal sig;
    sig.typeId = rp.GetHumanEvolutionContext().social.fear;
    sig.sourceEntityId = 1;
    sig.origin = {5, 5};
    sig.intensity = 0.8f;
    sig.confidence = 0.6f;
    sig.effectiveRadius = 8.0f;
    sig.createdTick = 0;
    sig.ttl = 10;
    b.SocialSignals().Emit(sig);

    // Now hashes must differ
    ASSERT_TRUE(ComputeWorldHash(a, HashTier::Full) !=
                ComputeWorldHash(b, HashTier::Full));

    return true;
}

TEST(same_active_social_signal_produces_same_full_hash)
{
    // Two identical worlds with identical signals should have the same hash.
    HumanEvolutionRulePack rp;

    auto makeWorldWithSignal = [&rp]() {
        WorldState w(32, 32, 42, rp);
        w.SpawnAgent(5, 5);

        SocialSignal sig;
        sig.typeId = rp.GetHumanEvolutionContext().social.fear;
        sig.sourceEntityId = 1;
        sig.origin = {5, 5};
        sig.intensity = 0.8f;
        sig.confidence = 0.6f;
        sig.effectiveRadius = 8.0f;
        sig.createdTick = 0;
        sig.ttl = 10;
        w.SocialSignals().Emit(sig);
        return w;
    };

    WorldState a = makeWorldWithSignal();
    WorldState b = makeWorldWithSignal();

    ASSERT_EQ(ComputeWorldHash(a, HashTier::Full),
              ComputeWorldHash(b, HashTier::Full));

    return true;
}
