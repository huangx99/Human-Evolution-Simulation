# Human Evolution Simulation

纯 C++17 确定性文明模拟器。零引擎依赖，零第三方库，种子可复现。

---

## 构建

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

运行测试：`./build/human_evolution_tests`

运行模拟：`./build/human_evolution_sim`

当前 43 个测试全过。

---

## 架构总览

```
WorldState（世界状态中心）
├── ModuleRegistry（模块注册表）
│   ├── SimulationModule   — 时钟、随机数、种子
│   ├── EnvironmentModule  — 温度、湿度、火场（Field2D 双缓冲）
│   ├── InformationModule  — 气味、危险、烟雾（Field2D 双缓冲）
│   ├── AgentModule        — 智能体列表
│   └── EcologyModule      — 生态实体注册表
├── EventBus               — 事件发射→分发→归档
├── CommandBuffer          — 命令提交→统一执行→历史记录
└── SpatialIndex           — Chunk 空间索引
```

### 核心设计原则

1. **确定性**：同一种子 → 同一结果。PRNG 用 Xoroshiro128+
2. **双缓冲**：Field2D 有 current/next 两个 Grid，读 current 写 next，tick 结束 Swap
3. **Command 模式**：系统不直接修改状态，提交 Command 由 CommandBuffer 统一执行
4. **语义驱动**：反应系统不认物品名，只认 Capability/State/Material/Affordance/Field
5. **模块化**：WorldState 通过 ModuleRegistry 注册/获取模块，类型安全

---

## 目录结构

```
core/                       — 基础设施（零依赖）
├── container/              — Grid<T>, Field2D<T>（双缓冲网格）
├── random/                 — Xoroshiro128+ PRNG
├── time/                   — SimClock
└── types/                  — i32, f32, u8, u16, u32, u64, EntityId, Tick

sim/                        — 模拟层
├── command/                — Command + CommandBuffer（统一命令执行）
├── ecology/                — 生态语义层
│   ├── material_id.h       — 材料枚举（Stone, Wood, Grass, Coal...）
│   ├── material_state.h    — 状态位域（Dry, Wet, Burning, Dead...）
│   ├── capability.h        — 能力位域（Flammable, Edible, HeatEmission...）
│   ├── affordance.h        — 可供性位域（CanIgnite, CanEat, CanBuild...）
│   ├── material_db.h       — 材料→默认属性的静态数据库
│   ├── ecology_entity.h    — 生态实体（Material + State + extra Capability/Affordance）
│   ├── ecology_registry.h  — 实体注册表（deque 存储，指针稳定）
│   └── field_id.h          — 场枚举（Temperature, Humidity, Fire, Smell...）
├── event/                  — EventBus（Emit→Dispatch→Archive）
├── scheduler/              — Phase Scheduler（12 阶段顺序执行）
├── spatial/                — 空间索引（Chunk Grid，接口层）
├── system/                 — 系统实现
│   ├── i_system.h          — ISystem 接口（Update(WorldState&)）
│   ├── climate_system.h    — 气候扩散
│   ├── fire_system.h       — 火焰传播
│   ├── smell_system.h      — 气味扩散
│   ├── agent_perception_system.h  — 感知
│   ├── agent_decision_system.h    — 决策
│   └── agent_action_system.h      — 行动（通过 Command）
└── world/                  — WorldState + 各 Module 定义

rules/                      — 规则层
└── reaction/               — 语义反应系统
    ├── semantic_predicate.h      — 9 种谓词类型
    ├── reaction_effect.h         — 8 种效果类型
    ├── semantic_reaction_rule.h  — 规则 = 谓词 + 效果
    └── semantic_reaction_system.h — 反应执行器

api/                        — 外部接口
├── snapshot/               — 世界快照（序列化）
└── replay/                 — 回放日志

app/                        — 应用层
├── headless_console/main.cpp — 无头模拟入口
└── tests/                  — 测试（43 个）
```

---

## 关键概念

### Field2D 双缓冲

```cpp
Field2D<f32> temperature;  // 内部有 current 和 next 两个 Grid

// 读：从 current
f32 val = temperature.At(x, y);

// 写：到 next
temperature.WriteNext(x, y) = 42.0f;

// tick 结束交换
temperature.Swap();
```

所有环境场（温度、湿度、火、气味、危险、烟雾）都用这个模式。

### Command 模式

```cpp
Command cmd;
cmd.type = CommandType::AddEntityState;
cmd.entityId = entity.id;
cmd.value = static_cast<f32>(static_cast<u32>(MaterialState::Burning));
world.commands.Submit(cmd);

// 统一执行（Scheduler 在 CommandApply 阶段自动调用）
world.commands.Apply(world);
```

CommandBuffer::Apply() 执行完命令后，如果检测到 ecology 实体变更，自动重建 SpatialIndex。

### 语义反应系统

规则由**谓词**和**效果**组成，系统不认具体物品名：

```cpp
// 谓词：只看语义属性
HasCapability(Flammable)     // 有可燃能力
HasState(Dry)                // 有干燥状态
NearbyCapability(HeatEmission, radius=2)  // 附近有热源
FieldGreaterThan(Temperature, 25.0f)      // 温度场 > 25

// 效果：通过 CommandBuffer 执行
AddState(Burning)
AddCapability(HeatEmission)
EmitSmell(delta=5.0f)
```

新增材料只需在 MaterialDB 加条目，规则用 Capability/State 描述，SemanticReactionSystem 零改动。

### Phase Scheduler

12 个阶段按序执行：

```
BeginTick → Input → CommandPrepare → Environment → Reaction →
Propagation → Perception → Decision → Action → CommandApply →
EventResolve → Snapshot → EndTick
```

系统注册到某个阶段，Scheduler 自动按序执行。每个阶段有内置行为：
- **BeginTick**：检查 SpatialIndex 是否已初始化，未初始化则自动 Rebuild
- **CommandApply**：执行 CommandBuffer（ecology 命令后自动重建 SpatialIndex）
- **EventResolve**：分发 EventBus 中的事件
- **EndTick**：Swap 所有 Field2D 双缓冲，时钟步进

CommandApply 在 EventResolve 前执行，确保事件处理器看到的是命令执行后的世界状态。

### 生态语义层

```
MaterialId   — 材料类型（Stone, Wood, Grass, Coal...）
MaterialState — 状态位域（Dry, Wet, Burning, Dead, Decomposing...）
Capability   — 能力位域（Flammable, Edible, HeatEmission, Buildable...）
Affordance   — 可供性位域（CanIgnite, CanEat, CanBuild...）
```

EcologyEntity 组合这些属性：
- `material` → MaterialDB 查默认 Capability/Affordance
- `state` → 当前状态（可叠加，位域运算）
- `extraCapabilities` / `extraAffordances` → 运行时动态添加

---

## 如何扩展

### 新增材料

1. `sim/ecology/material_id.h` — 加枚举值
2. `sim/ecology/material_db.h` — 加默认属性

```cpp
// material_id.h
Coal,

// material_db.h
db[static_cast<size_t>(MaterialId::Coal)] = {
    Capability::Flammable | Capability::HeatEmission,
    Affordance::CanIgnite | Affordance::CanBuild,
    MaterialState::Dry
};
```

### 新增反应规则

```cpp
SemanticReactionRule rule;
rule.id = "coal_burn";
rule.name = "Coal burns when heated";

SemanticPredicate isFlammable;
isFlammable.type = PredicateType::HasCapability;
isFlammable.capability = Capability::Flammable;

SemanticPredicate isDry;
isDry.type = PredicateType::HasState;
isDry.state = MaterialState::Dry;

SemanticPredicate nearbyHeat;
nearbyHeat.type = PredicateType::NearbyCapability;
nearbyHeat.capability = Capability::HeatEmission;
nearbyHeat.radius = 2;

rule.conditions = {isFlammable, isDry, nearbyHeat};

ReactionEffect addBurning;
addBurning.type = EffectType::AddState;
addBurning.state = MaterialState::Burning;

rule.effects = {addBurning};

sys.AddRule(rule);
```

ModifyField 效果使用 `FieldModifyMode` 明确语义：

```cpp
ReactionEffect modTemp;
modTemp.type = EffectType::ModifyField;
modTemp.field = FieldId::Temperature;
modTemp.mode = FieldModifyMode::Add;   // 或 FieldModifyMode::Set
modTemp.value = 5.0f;
```

### 新增谓词类型

1. `rules/reaction/semantic_predicate.h` — 加 PredicateType 枚举
2. `rules/reaction/semantic_reaction_system.h` — EvaluatePredicate 加 case

### 新增效果类型

1. `rules/reaction/reaction_effect.h` — 加 EffectType 枚举
2. `sim/command/command.h` — 加 CommandType
3. `sim/command/command_buffer.cpp` — Execute 加 case
4. `rules/reaction/semantic_reaction_system.h` — SubmitEffects 加 case

---

## 已知限制

- **fire_system / smell_system** 直接写 Field2D（双缓冲模式），未走 CommandBuffer。这是环境场系统的当前设计，与 ecology entity 的 Command 模式是不同层
- **agent_perception / agent_decision** 直接修改 Agent 字段，未走 Command。agent_action 已走 Command
- **SpatialIndex** 是基础索引层，非完整性能优化。DirtyRegion 为 step 2 预留
- **所有系统都是单线程**，无并行执行

---

## 测试

```
43 tests, 0 failures

基础测试：Grid、Random、Determinism
架构测试：ModuleRegistry、Field2D、Scheduler、Command、Event、Snapshot
生态测试：MaterialDB、State/Capability/Affordance 位域、Registry 查询
语义测试：谓词类型、效果类型、规则构建
执行测试：高温石头点火、火把点火、湿木不燃、腐肉产味、湿草不燃
扩展测试：Coal 点火、RottingPlant 产味、Coal 不燃湿草
```

---

## 接手建议

1. **先跑测试**：`cmake --build build && ./build/human_evolution_tests`，确认 43 全过
2. **读代码顺序**：WorldState → EcologyEntity → MaterialDB → SemanticReactionSystem → CommandBuffer
3. **扩展时不要改 SemanticReactionSystem**：新增材料/规则只改数据层
4. **环境场系统（fire/smell/climate）是独立层**：它们用双缓冲直接写 Field2D，不走 Command
5. **下一步方向**：Event Integration（生态变化进入历史）、Knowledge/Discovery 系统、Spatial DirtyRegion
