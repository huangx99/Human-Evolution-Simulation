#pragma once

// Polymorphic Command system — engine defines CommandBase + core commands.
// World-specific commands (IgniteFire, EmitSmell, etc.) live in rules/.
//
// === ARCHITECTURE RULES (Runtime Laws) ===
//
// Rule 1: Phase Barrier Visibility
//   Commands become visible only after phase barrier.
//   Same phase systems CANNOT see each other's commands.
//   Phase end: unified apply. Next phase: commands visible.
//
// Rule 2: Stable Command Identity
//   Every command has a stable CommandKind : u16 ID that NEVER changes.
//   Used for serialization/replay/hash — never raw type info.
//
// Rule 3: No Source-of-Truth Duplication
//   Commands store only the intent (target + delta), not current state.
//   WorldState is the single source of truth.
//
// Rule 4: Engine commands only — world commands in rules/
//   Engine: MoveAgent, SetAgentAction, DamageAgent, FeedAgent, ModifyHunger,
//           AddEntityState, RemoveEntityState, AddEntityCapability,
//           RemoveEntityCapability, ModifyFieldValue
//   World:  IgniteFire, ExtinguishFire, EmitSmell, SetDanger, EmitSmoke

#include "core/types/types.h"
#include "sim/entity/agent_action.h"
#include "sim/field/field_id.h"
#include "sim/field/field_module.h"
#include <memory>
#include <cstring>

struct WorldState;

// === Stable command identity (for serialization/replay) ===
enum class CommandKind : u16
{
    // Agent
    MoveAgent = 1,
    SetAgentAction = 2,
    DamageAgent = 3,
    FeedAgent = 4,
    ModifyHunger = 5,

    // Ecology entity
    AddEntityState = 30,
    RemoveEntityState = 31,
    AddEntityCapability = 32,
    RemoveEntityCapability = 33,
    ModifyFieldValue = 34,

    // World-specific commands start at 100
    // (IgniteFire=100, ExtinguishFire=101, etc. — defined in rules/)
};

// === Polymorphic command base ===
//
// All commands inherit from CommandBase. The engine defines core commands;
// world-specific commands are registered by RulePacks.

class CommandBase
{
public:
    virtual ~CommandBase() = default;

    // Execute this command on the world
    virtual void Execute(WorldState& world) const = 0;

    // Deep copy (for replay history)
    virtual std::unique_ptr<CommandBase> Clone() const = 0;

    // Stable kind ID (for serialization/replay/hash)
    virtual CommandKind GetKind() const = 0;

    // Serialize command parameters to buffer (for replay persistence)
    // Default: no data (commands with no parameters)
    virtual void SerializeData(std::vector<u8>&) const {}

    // Hash command parameters (for determinism verification)
    virtual void FeedHash(class SimHash&) const {}
};

// === Engine command structs ===

struct MoveAgentCommand : public CommandBase
{
    EntityId id = 0;
    i32 x = 0, y = 0;

    MoveAgentCommand() = default;
    MoveAgentCommand(EntityId id, i32 x, i32 y) : id(id), x(x), y(y) {}

    void Execute(WorldState& world) const override;
    std::unique_ptr<CommandBase> Clone() const override { return std::make_unique<MoveAgentCommand>(*this); }
    CommandKind GetKind() const override { return CommandKind::MoveAgent; }
    void SerializeData(std::vector<u8>& buf) const override;
    void FeedHash(class SimHash& h) const override;
};

struct SetAgentActionCommand : public CommandBase
{
    EntityId id = 0;
    AgentAction action = AgentAction::Idle;

    SetAgentActionCommand() = default;
    SetAgentActionCommand(EntityId id, AgentAction action) : id(id), action(action) {}

    void Execute(WorldState& world) const override;
    std::unique_ptr<CommandBase> Clone() const override { return std::make_unique<SetAgentActionCommand>(*this); }
    CommandKind GetKind() const override { return CommandKind::SetAgentAction; }
    void SerializeData(std::vector<u8>& buf) const override;
    void FeedHash(class SimHash& h) const override;
};

struct DamageAgentCommand : public CommandBase
{
    EntityId id = 0;
    f32 amount = 0.0f;

    DamageAgentCommand() = default;
    DamageAgentCommand(EntityId id, f32 amount) : id(id), amount(amount) {}

    void Execute(WorldState& world) const override;
    std::unique_ptr<CommandBase> Clone() const override { return std::make_unique<DamageAgentCommand>(*this); }
    CommandKind GetKind() const override { return CommandKind::DamageAgent; }
    void SerializeData(std::vector<u8>& buf) const override;
    void FeedHash(class SimHash& h) const override;
};

struct FeedAgentCommand : public CommandBase
{
    EntityId id = 0;
    f32 amount = 0.0f;

    FeedAgentCommand() = default;
    FeedAgentCommand(EntityId id, f32 amount) : id(id), amount(amount) {}

    void Execute(WorldState& world) const override;
    std::unique_ptr<CommandBase> Clone() const override { return std::make_unique<FeedAgentCommand>(*this); }
    CommandKind GetKind() const override { return CommandKind::FeedAgent; }
    void SerializeData(std::vector<u8>& buf) const override;
    void FeedHash(class SimHash& h) const override;
};

struct ModifyHungerCommand : public CommandBase
{
    EntityId id = 0;
    f32 delta = 0.0f;

    ModifyHungerCommand() = default;
    ModifyHungerCommand(EntityId id, f32 delta) : id(id), delta(delta) {}

    void Execute(WorldState& world) const override;
    std::unique_ptr<CommandBase> Clone() const override { return std::make_unique<ModifyHungerCommand>(*this); }
    CommandKind GetKind() const override { return CommandKind::ModifyHunger; }
    void SerializeData(std::vector<u8>& buf) const override;
    void FeedHash(class SimHash& h) const override;
};

struct AddEntityStateCommand : public CommandBase
{
    EntityId id = 0;
    u32 state = 0;

    AddEntityStateCommand() = default;
    AddEntityStateCommand(EntityId id, u32 state) : id(id), state(state) {}

    void Execute(WorldState& world) const override;
    std::unique_ptr<CommandBase> Clone() const override { return std::make_unique<AddEntityStateCommand>(*this); }
    CommandKind GetKind() const override { return CommandKind::AddEntityState; }
    void SerializeData(std::vector<u8>& buf) const override;
    void FeedHash(class SimHash& h) const override;
};

struct RemoveEntityStateCommand : public CommandBase
{
    EntityId id = 0;
    u32 state = 0;

    RemoveEntityStateCommand() = default;
    RemoveEntityStateCommand(EntityId id, u32 state) : id(id), state(state) {}

    void Execute(WorldState& world) const override;
    std::unique_ptr<CommandBase> Clone() const override { return std::make_unique<RemoveEntityStateCommand>(*this); }
    CommandKind GetKind() const override { return CommandKind::RemoveEntityState; }
    void SerializeData(std::vector<u8>& buf) const override;
    void FeedHash(class SimHash& h) const override;
};

struct AddEntityCapabilityCommand : public CommandBase
{
    EntityId id = 0;
    u32 capability = 0;

    AddEntityCapabilityCommand() = default;
    AddEntityCapabilityCommand(EntityId id, u32 cap) : id(id), capability(cap) {}

    void Execute(WorldState& world) const override;
    std::unique_ptr<CommandBase> Clone() const override { return std::make_unique<AddEntityCapabilityCommand>(*this); }
    CommandKind GetKind() const override { return CommandKind::AddEntityCapability; }
    void SerializeData(std::vector<u8>& buf) const override;
    void FeedHash(class SimHash& h) const override;
};

struct RemoveEntityCapabilityCommand : public CommandBase
{
    EntityId id = 0;
    u32 capability = 0;

    RemoveEntityCapabilityCommand() = default;
    RemoveEntityCapabilityCommand(EntityId id, u32 cap) : id(id), capability(cap) {}

    void Execute(WorldState& world) const override;
    std::unique_ptr<CommandBase> Clone() const override { return std::make_unique<RemoveEntityCapabilityCommand>(*this); }
    CommandKind GetKind() const override { return CommandKind::RemoveEntityCapability; }
    void SerializeData(std::vector<u8>& buf) const override;
    void FeedHash(class SimHash& h) const override;
};

struct ModifyFieldValueCommand : public CommandBase
{
    i32 x = 0, y = 0;
    FieldKey field;
    i32 mode = 0;  // 0=add, 1=set
    f32 value = 0.0f;

    ModifyFieldValueCommand() = default;
    ModifyFieldValueCommand(i32 x, i32 y, FieldKey field, i32 mode, f32 value)
        : x(x), y(y), field(field), mode(mode), value(value) {}

    void Execute(WorldState& world) const override;
    std::unique_ptr<CommandBase> Clone() const override { return std::make_unique<ModifyFieldValueCommand>(*this); }
    CommandKind GetKind() const override { return CommandKind::ModifyFieldValue; }
    void SerializeData(std::vector<u8>& buf) const override;
    void FeedHash(class SimHash& h) const override;
};

// === CommandFactory: extensible command deserialization ===
//
// Engine registers core factories; RulePacks register world-specific ones.

using CommandFactory = std::unique_ptr<CommandBase>(*)(const u8* data, size_t size);

class CommandRegistry
{
public:
    static CommandRegistry& Instance()
    {
        static CommandRegistry reg;
        return reg;
    }

    void Register(CommandKind kind, CommandFactory factory)
    {
        factories_[static_cast<u16>(kind)] = factory;
    }

    std::unique_ptr<CommandBase> Create(CommandKind kind, const u8* data, size_t size) const
    {
        auto idx = static_cast<u16>(kind);
        if (idx < MAX_KINDS && factories_[idx])
            return factories_[idx](data, size);
        return nullptr;
    }

private:
    static constexpr size_t MAX_KINDS = 256;
    CommandFactory factories_[MAX_KINDS] = {};

    CommandRegistry();
};

// === QueuedCommand: command + tick for history ===
struct QueuedCommand
{
    Tick tick;
    std::unique_ptr<CommandBase> command;

    QueuedCommand() = default;
    QueuedCommand(Tick t, std::unique_ptr<CommandBase> c) : tick(t), command(std::move(c)) {}

    // Move-only
    QueuedCommand(QueuedCommand&&) = default;
    QueuedCommand& operator=(QueuedCommand&&) = default;
    QueuedCommand(const QueuedCommand& o) : tick(o.tick), command(o.command ? o.command->Clone() : nullptr) {}
    QueuedCommand& operator=(const QueuedCommand& o)
    {
        tick = o.tick;
        command = o.command ? o.command->Clone() : nullptr;
        return *this;
    }
};

// === Helper: append typed data to buffer ===
template<typename T>
inline void AppendBytes(std::vector<u8>& buf, const T& val)
{
    const u8* p = reinterpret_cast<const u8*>(&val);
    buf.insert(buf.end(), p, p + sizeof(T));
}

template<typename T>
inline T ReadBytes(const u8*& data, size_t& remaining)
{
    T val;
    std::memcpy(&val, data, sizeof(T));
    data += sizeof(T);
    remaining -= sizeof(T);
    return val;
}
