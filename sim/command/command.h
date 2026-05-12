#pragma once

// Typed Command system — replaces the generic Command struct with type-safe variants.
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
//   Variant index is an implementation detail — serialization/replay/network
//   must use CommandKind, never std::variant::index().
//
// Rule 3: No Source-of-Truth Duplication
//   Commands store only the intent (target + delta), not current state.
//   WorldState is the single source of truth.

#include "core/types/types.h"
#include "sim/entity/agent_action.h"
#include "sim/ecology/field_id.h"
#include <variant>
#include <type_traits>

// === Stable command identity (for serialization/replay) ===
enum class CommandKind : u16
{
    // Agent
    MoveAgent = 1,
    SetAgentAction = 2,
    DamageAgent = 3,
    FeedAgent = 4,
    ModifyHunger = 5,

    // Environment
    IgniteFire = 10,
    ExtinguishFire = 11,
    EmitSmell = 12,

    // Information
    SetDanger = 20,

    // Ecology entity
    AddEntityState = 30,
    RemoveEntityState = 31,
    AddEntityCapability = 32,
    RemoveEntityCapability = 33,
    ModifyFieldValue = 34,
    EmitSmoke = 35,
};

// === Typed command structs ===

// Agent commands
struct MoveAgentCommand      { EntityId id; i32 x, y; };
struct SetAgentActionCommand { EntityId id; AgentAction action; };
struct DamageAgentCommand    { EntityId id; f32 amount; };
struct FeedAgentCommand      { EntityId id; f32 amount; };
struct ModifyHungerCommand   { EntityId id; f32 delta; };

// Environment commands
struct IgniteFireCommand     { i32 x, y; f32 intensity; };
struct ExtinguishFireCommand { i32 x, y; };
struct EmitSmellCommand      { i32 x, y; f32 amount; };

// Information commands
struct SetDangerCommand      { i32 x, y; f32 value; };

// Ecology entity commands
struct AddEntityStateCommand         { EntityId id; u32 state; };
struct RemoveEntityStateCommand      { EntityId id; u32 state; };
struct AddEntityCapabilityCommand    { EntityId id; u32 capability; };
struct RemoveEntityCapabilityCommand { EntityId id; u32 capability; };
struct ModifyFieldValueCommand       { i32 x, y; FieldId field; i32 mode; f32 value; };
struct EmitSmokeCommand              { i32 x, y; f32 amount; };

// === Sub-variants by domain (prevents type explosion) ===
using AgentCommand = std::variant<
    MoveAgentCommand, SetAgentActionCommand, DamageAgentCommand,
    FeedAgentCommand, ModifyHungerCommand
>;

using EnvironmentCommand = std::variant<
    IgniteFireCommand, ExtinguishFireCommand, EmitSmellCommand, SetDangerCommand
>;

using EcologyCommand = std::variant<
    AddEntityStateCommand, RemoveEntityStateCommand,
    AddEntityCapabilityCommand, RemoveEntityCapabilityCommand,
    ModifyFieldValueCommand, EmitSmokeCommand
>;

// === Top-level Command ===
using Command = std::variant<
    AgentCommand, EnvironmentCommand, EcologyCommand
>;

// === QueuedCommand: command + tick for history ===
struct QueuedCommand
{
    Tick tick;
    Command command;
};

// === MakeCommand: centralized wrapping (one overload per command type) ===
// Adding a new command type? Add one MakeCommand overload here. CommandBuffer stays untouched.

inline Command MakeCommand(MoveAgentCommand cmd)      { return AgentCommand{cmd}; }
inline Command MakeCommand(SetAgentActionCommand cmd)  { return AgentCommand{cmd}; }
inline Command MakeCommand(DamageAgentCommand cmd)     { return AgentCommand{cmd}; }
inline Command MakeCommand(FeedAgentCommand cmd)       { return AgentCommand{cmd}; }
inline Command MakeCommand(ModifyHungerCommand cmd)    { return AgentCommand{cmd}; }

inline Command MakeCommand(IgniteFireCommand cmd)      { return EnvironmentCommand{cmd}; }
inline Command MakeCommand(ExtinguishFireCommand cmd)  { return EnvironmentCommand{cmd}; }
inline Command MakeCommand(EmitSmellCommand cmd)       { return EnvironmentCommand{cmd}; }
inline Command MakeCommand(SetDangerCommand cmd)       { return EnvironmentCommand{cmd}; }

inline Command MakeCommand(AddEntityStateCommand cmd)         { return EcologyCommand{cmd}; }
inline Command MakeCommand(RemoveEntityStateCommand cmd)      { return EcologyCommand{cmd}; }
inline Command MakeCommand(AddEntityCapabilityCommand cmd)    { return EcologyCommand{cmd}; }
inline Command MakeCommand(RemoveEntityCapabilityCommand cmd) { return EcologyCommand{cmd}; }
inline Command MakeCommand(ModifyFieldValueCommand cmd)       { return EcologyCommand{cmd}; }
inline Command MakeCommand(EmitSmokeCommand cmd)              { return EcologyCommand{cmd}; }

// === IsCommandType trait: type constraint for template Submit ===
template<typename T> struct IsCommandType : std::false_type {};

template<> struct IsCommandType<MoveAgentCommand>      : std::true_type {};
template<> struct IsCommandType<SetAgentActionCommand>  : std::true_type {};
template<> struct IsCommandType<DamageAgentCommand>     : std::true_type {};
template<> struct IsCommandType<FeedAgentCommand>       : std::true_type {};
template<> struct IsCommandType<ModifyHungerCommand>    : std::true_type {};
template<> struct IsCommandType<IgniteFireCommand>      : std::true_type {};
template<> struct IsCommandType<ExtinguishFireCommand>  : std::true_type {};
template<> struct IsCommandType<EmitSmellCommand>       : std::true_type {};
template<> struct IsCommandType<SetDangerCommand>       : std::true_type {};
template<> struct IsCommandType<AddEntityStateCommand>         : std::true_type {};
template<> struct IsCommandType<RemoveEntityStateCommand>      : std::true_type {};
template<> struct IsCommandType<AddEntityCapabilityCommand>    : std::true_type {};
template<> struct IsCommandType<RemoveEntityCapabilityCommand> : std::true_type {};
template<> struct IsCommandType<ModifyFieldValueCommand>       : std::true_type {};
template<> struct IsCommandType<EmitSmokeCommand>              : std::true_type {};

// === GetCommandKind: stable ID from Command (for serialization/replay) ===
inline CommandKind GetCommandKind(const Command& cmd)
{
    return std::visit([](const auto& inner) -> CommandKind {
        return std::visit([](const auto& typed) -> CommandKind {
            using T = std::decay_t<decltype(typed)>;
            if constexpr (std::is_same_v<T, MoveAgentCommand>)             return CommandKind::MoveAgent;
            else if constexpr (std::is_same_v<T, SetAgentActionCommand>)   return CommandKind::SetAgentAction;
            else if constexpr (std::is_same_v<T, DamageAgentCommand>)      return CommandKind::DamageAgent;
            else if constexpr (std::is_same_v<T, FeedAgentCommand>)        return CommandKind::FeedAgent;
            else if constexpr (std::is_same_v<T, ModifyHungerCommand>)     return CommandKind::ModifyHunger;
            else if constexpr (std::is_same_v<T, IgniteFireCommand>)       return CommandKind::IgniteFire;
            else if constexpr (std::is_same_v<T, ExtinguishFireCommand>)   return CommandKind::ExtinguishFire;
            else if constexpr (std::is_same_v<T, EmitSmellCommand>)        return CommandKind::EmitSmell;
            else if constexpr (std::is_same_v<T, SetDangerCommand>)        return CommandKind::SetDanger;
            else if constexpr (std::is_same_v<T, AddEntityStateCommand>)         return CommandKind::AddEntityState;
            else if constexpr (std::is_same_v<T, RemoveEntityStateCommand>)      return CommandKind::RemoveEntityState;
            else if constexpr (std::is_same_v<T, AddEntityCapabilityCommand>)    return CommandKind::AddEntityCapability;
            else if constexpr (std::is_same_v<T, RemoveEntityCapabilityCommand>) return CommandKind::RemoveEntityCapability;
            else if constexpr (std::is_same_v<T, ModifyFieldValueCommand>)       return CommandKind::ModifyFieldValue;
            else if constexpr (std::is_same_v<T, EmitSmokeCommand>)              return CommandKind::EmitSmoke;
        }, inner);
    }, cmd);
}
