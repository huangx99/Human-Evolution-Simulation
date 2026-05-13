#pragma once

#include "sim/command/command.h"
#include "sim/world/world_state.h"
#include "sim/runtime/simulation_hash.h"

// HumanEvolution world commands — NOT in the engine layer.
// These commands are specific to the Human Evolution world rules.

// === Command kind IDs (start at 100, engine uses 1-34) ===
namespace HumanEvolution {
namespace CmdKind {
    constexpr CommandKind IgniteFire      = static_cast<CommandKind>(100);
    constexpr CommandKind ExtinguishFire  = static_cast<CommandKind>(101);
    constexpr CommandKind EmitSmell       = static_cast<CommandKind>(102);
    constexpr CommandKind SetDanger       = static_cast<CommandKind>(103);
    constexpr CommandKind EmitSmoke       = static_cast<CommandKind>(104);
}
}

// === World command structs ===

struct IgniteFireCommand : public CommandBase
{
    i32 x = 0, y = 0;
    f32 intensity = 0.0f;

    IgniteFireCommand() = default;
    IgniteFireCommand(i32 x, i32 y, f32 intensity) : x(x), y(y), intensity(intensity) {}

    void Execute(WorldState& world) const override
    {
        if (world.Env().env2.InBounds(x, y))
            world.Env().env2.WriteNext(x, y) = intensity;
    }

    std::unique_ptr<CommandBase> Clone() const override { return std::make_unique<IgniteFireCommand>(*this); }
    CommandKind GetKind() const override { return HumanEvolution::CmdKind::IgniteFire; }

    void SerializeData(std::vector<u8>& buf) const override
    {
        AppendBytes(buf, x); AppendBytes(buf, y); AppendBytes(buf, intensity);
    }

    void FeedHash(SimHash& h) const override
    {
        h.FeedI32(x); h.FeedI32(y); h.FeedF32(intensity);
    }
};

struct ExtinguishFireCommand : public CommandBase
{
    i32 x = 0, y = 0;

    ExtinguishFireCommand() = default;
    ExtinguishFireCommand(i32 x, i32 y) : x(x), y(y) {}

    void Execute(WorldState& world) const override
    {
        if (world.Env().env2.InBounds(x, y))
            world.Env().env2.WriteNext(x, y) = 0.0f;
    }

    std::unique_ptr<CommandBase> Clone() const override { return std::make_unique<ExtinguishFireCommand>(*this); }
    CommandKind GetKind() const override { return HumanEvolution::CmdKind::ExtinguishFire; }

    void SerializeData(std::vector<u8>& buf) const override
    {
        AppendBytes(buf, x); AppendBytes(buf, y);
    }

    void FeedHash(SimHash& h) const override
    {
        h.FeedI32(x); h.FeedI32(y);
    }
};

struct EmitSmellCommand : public CommandBase
{
    i32 x = 0, y = 0;
    f32 amount = 0.0f;

    EmitSmellCommand() = default;
    EmitSmellCommand(i32 x, i32 y, f32 amount) : x(x), y(y), amount(amount) {}

    void Execute(WorldState& world) const override
    {
        if (world.Info().info0.InBounds(x, y))
            world.Info().info0.WriteNext(x, y) += amount;
    }

    std::unique_ptr<CommandBase> Clone() const override { return std::make_unique<EmitSmellCommand>(*this); }
    CommandKind GetKind() const override { return HumanEvolution::CmdKind::EmitSmell; }

    void SerializeData(std::vector<u8>& buf) const override
    {
        AppendBytes(buf, x); AppendBytes(buf, y); AppendBytes(buf, amount);
    }

    void FeedHash(SimHash& h) const override
    {
        h.FeedI32(x); h.FeedI32(y); h.FeedF32(amount);
    }
};

struct SetDangerCommand : public CommandBase
{
    i32 x = 0, y = 0;
    f32 value = 0.0f;

    SetDangerCommand() = default;
    SetDangerCommand(i32 x, i32 y, f32 value) : x(x), y(y), value(value) {}

    void Execute(WorldState& world) const override
    {
        if (world.Info().info1.InBounds(x, y))
            world.Info().info1.WriteNext(x, y) = value;
    }

    std::unique_ptr<CommandBase> Clone() const override { return std::make_unique<SetDangerCommand>(*this); }
    CommandKind GetKind() const override { return HumanEvolution::CmdKind::SetDanger; }

    void SerializeData(std::vector<u8>& buf) const override
    {
        AppendBytes(buf, x); AppendBytes(buf, y); AppendBytes(buf, value);
    }

    void FeedHash(SimHash& h) const override
    {
        h.FeedI32(x); h.FeedI32(y); h.FeedF32(value);
    }
};

struct EmitSmokeCommand : public CommandBase
{
    i32 x = 0, y = 0;
    f32 amount = 0.0f;

    EmitSmokeCommand() = default;
    EmitSmokeCommand(i32 x, i32 y, f32 amount) : x(x), y(y), amount(amount) {}

    void Execute(WorldState& world) const override
    {
        if (world.Info().info2.InBounds(x, y))
            world.Info().info2.WriteNext(x, y) += amount;
    }

    std::unique_ptr<CommandBase> Clone() const override { return std::make_unique<EmitSmokeCommand>(*this); }
    CommandKind GetKind() const override { return HumanEvolution::CmdKind::EmitSmoke; }

    void SerializeData(std::vector<u8>& buf) const override
    {
        AppendBytes(buf, x); AppendBytes(buf, y); AppendBytes(buf, amount);
    }

    void FeedHash(SimHash& h) const override
    {
        h.FeedI32(x); h.FeedI32(y); h.FeedF32(amount);
    }
};

// === Register world command factories ===

inline void RegisterHumanEvolutionCommands()
{
    auto& reg = CommandRegistry::Instance();

    reg.Register(HumanEvolution::CmdKind::IgniteFire, [](const u8* data, size_t size) -> std::unique_ptr<CommandBase> {
        auto x = ReadBytes<i32>(data, size);
        auto y = ReadBytes<i32>(data, size);
        auto intensity = ReadBytes<f32>(data, size);
        auto cmd = std::make_unique<IgniteFireCommand>();
        cmd->x = x; cmd->y = y; cmd->intensity = intensity;
        return cmd;
    });

    reg.Register(HumanEvolution::CmdKind::ExtinguishFire, [](const u8* data, size_t size) -> std::unique_ptr<CommandBase> {
        auto x = ReadBytes<i32>(data, size);
        auto y = ReadBytes<i32>(data, size);
        auto cmd = std::make_unique<ExtinguishFireCommand>();
        cmd->x = x; cmd->y = y;
        return cmd;
    });

    reg.Register(HumanEvolution::CmdKind::EmitSmell, [](const u8* data, size_t size) -> std::unique_ptr<CommandBase> {
        auto x = ReadBytes<i32>(data, size);
        auto y = ReadBytes<i32>(data, size);
        auto amount = ReadBytes<f32>(data, size);
        auto cmd = std::make_unique<EmitSmellCommand>();
        cmd->x = x; cmd->y = y; cmd->amount = amount;
        return cmd;
    });

    reg.Register(HumanEvolution::CmdKind::SetDanger, [](const u8* data, size_t size) -> std::unique_ptr<CommandBase> {
        auto x = ReadBytes<i32>(data, size);
        auto y = ReadBytes<i32>(data, size);
        auto value = ReadBytes<f32>(data, size);
        auto cmd = std::make_unique<SetDangerCommand>();
        cmd->x = x; cmd->y = y; cmd->value = value;
        return cmd;
    });

    reg.Register(HumanEvolution::CmdKind::EmitSmoke, [](const u8* data, size_t size) -> std::unique_ptr<CommandBase> {
        auto x = ReadBytes<i32>(data, size);
        auto y = ReadBytes<i32>(data, size);
        auto amount = ReadBytes<f32>(data, size);
        auto cmd = std::make_unique<EmitSmokeCommand>();
        cmd->x = x; cmd->y = y; cmd->amount = amount;
        return cmd;
    });
}
