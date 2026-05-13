# Runtime Law

These are the authoritative architecture rules for the Human Evolution Simulation.
Every rule exists because violating it will cause a specific class of bug.
Laws are numbered for reference; order does not imply priority.

---

## Law 1: Phase Barrier Visibility

```
Commands become visible only after phase barrier.
Same-phase systems CANNOT see each other's commands.
Phase end: unified apply. Next phase: commands visible.
```

**Why**: Systems within a phase must be logically independent. This is the foundation for future parallelism and for deterministic replay.

**Enforcement**: `CommandBuffer::Apply()` runs at `SimPhase::CommandApply`. Systems in `Action` phase submit commands; they are applied at phase end and visible in `EventResolve`.

---

## Law 2: Stable Command Identity

```
Every command inherits from CommandBase and has a stable CommandKind : u16 that NEVER changes.
Command is type-erased via CommandBase* — never depend on RTTI, typeid, or variant index.
Serialization, replay, and network must use CommandKind.
RulePack commands register factories via CommandRegistry.
```

**Why**: Commands use virtual dispatch (CommandBase + unique_ptr) rather than std::variant to support RulePack extension without modifying engine code. The CommandKind enum is the only stable identity — it persists across versions for replay, hash, and network. RTTI/typeid are non-portable and fragile across compilation units.

**Enforcement**: `CommandBase::GetKind()` returns the stable `CommandKind`. `CommandRegistry` maps `CommandKind` → deserialization factory. World-specific commands (IgniteFire, EmitSmell, etc.) are defined in `rules/` and registered by RulePack, never in `sim/`.

---

## Law 3: No Source-of-Truth Duplication

```
Commands store only the intent (target + delta), not current state.
WorldState is the single source of truth.
```

**Why**: If a command stores `fromX/fromY` and the agent moves before the command is applied, the stored source is stale. Commands must be self-contained descriptions of what to do, not snapshots of what was.

**Enforcement**: Code review. Command structs should never contain "current state" fields.

---

## Law 4: Memory Only From FocusedStimulus

```
All memory creation must go through CognitiveMemorySystem.
Only FocusedStimulus can become MemoryRecord.
```

**Why**: The attention bottleneck is what makes agents cognitively limited. If any system can inject memories directly, agents become omniscient and the cognitive model collapses.

**Enforcement**: `CognitiveMemorySystem` is the sole writer to `CognitiveModule::agentMemories`. No other system accesses this field.

---

## Law 5: Attention Bottleneck

```
CognitivePerceptionSystem is the sole reader of WorldState for cognitive data.
All downstream systems consume PerceivedStimulus or FocusedStimulus derivatives.
```

**Why**: If downstream cognitive systems read WorldState directly, they bypass the attention filter. The pipeline (Perception → Attention → Memory → Discovery → Knowledge → Behavior) must be strictly linear.

**Enforcement**: `CognitivePerceptionSystem` header documents this. Downstream systems only access `frameStimuli`, `frameFocused`, `frameMemories` — never raw WorldState fields.

---

## Law 6: Field vs Semantic Boundary

```
Field systems (fire, smell, climate) propagate numeric values.
Semantic systems (reactions, cognition) handle state transitions.
They must NOT cross-contaminate.
```

**Why**: Field systems operate on dense `Field2D<f32>` grids with `WriteNext`. Semantic systems operate on entity state via commands. Mixing them creates double-buffer timing bugs and spatial index corruption.

**Enforcement**: `i_system.h` documents the boundary. Field systems must NOT handle entity state transitions. Semantic systems must NOT write to Field2D directly.

---

## Law 7: Engine Does Not Know Rules

```
sim/ never includes rules/.
Dependency is one-directional: rules/ → sim/, never reverse.
RulePacks register via ISystem + Scheduler.
```

**Why**: If the engine hardcodes civilization-specific logic, adding new civilizations requires modifying the engine. The engine should be a generic simulation runtime; rules define what kind of world this is.

**Enforcement**: `grep -r "rules/" sim/` must return nothing. CMake targets enforce build-time boundary.

---

## Law 8: Replay Is Command Re-Apply

```
Replay does NOT re-run systems.
ReplayEngine directly applies recorded commands onto a snapshot.
ReplayEngine does NOT call CommandBuffer::Submit().
```

**Why**: Replay is history reconstruction, not re-entering the runtime pipeline. If replay runs systems, it re-consumes randomness, re-orders system execution, and re-triggers nondeterministic bugs. Commands are the proof of what happened; replay re-applies that proof.

**Enforcement**: `ReplayEngine::Replay()` iterates recorded commands and applies them directly to WorldState. No system execution, no Submit() calls.

---

## Law 9: Stable Hash Iteration Order

```
All hashed iteration order must be stable.
Never hash: pointers, unordered_map iteration order, allocator-dependent order.
Use sorted vectors or index-based iteration for hash input.
```

**Why**: `unordered_map` iteration order varies across platforms (Linux vs Windows), compilers (clang vs gcc), and even runs (ASLR affects bucket layout). If the hash includes unordered iteration, determinism proofs fail cross-platform.

**Enforcement**: Code review. Hash functions must iterate sorted vectors or index-based containers. `PatternModule` uses `vector<pair>` instead of `unordered_map` for this reason.
