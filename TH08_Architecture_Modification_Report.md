# 向《永夜抄》靠拢的架构改造报告

## 结论与目标

建议借鉴官作可观察到的组织方式，不照搬其二进制文件格式、对象内存布局或反编译出的偏移量。改造目标是：

1. 用关卡时钟和有序事件驱动出场，让刷怪、Boss 阶段、弹幕、背景等事件可以按脚本时间并行推进；清场等待只作为显式事件。
2. 将弹幕发射图案、视觉/判定弹种、单颗弹幕轨迹状态分开。图案产生一组弹幕初值，轨迹状态机逐帧演进每颗弹。
3. 将敌弹从“每颗 `new` + `vector` 擦除”迁移到固定槽位池，槽位复用；对外用句柄或只读遍历接口，不暴露拥有权。

项目启用 C++17，可直接使用 `std::variant`、`std::array`、`std::optional` 等标准库类型。推荐按下文的阶段逐步迁移，不要求一次重写所有游戏系统。

## 当前项目基线

| 子系统 | 当前实现 | 主要影响 |
|---|---|---|
| 波次 | `Game` 持有 `queue<waveData>`；`Game::HandleRound()` 在 `checkEnemyClear()` 成立后才为下一波计时。[Game.cpp:142](TouhouOOP/Game.cpp#L142) [EnemyManager.cpp:81](TouhouOOP/EnemyManager.cpp#L81) | 下一波是否开始依赖敌人和生成事件是否清空，不能直接表达“第 N 帧无论场上还有什么都触发”。 |
| 波内刷怪 | `SpawnEvent` 有 `startTime/count/interval`；`EnemyManager::updateSpawns()` 用 `GetTickCount()` 把到时的事件移入活跃列表，再按间隔生成。[EnemyManager.h:12](TouhouOOP/EnemyManager.h#L12) [EnemyManager.cpp:31](TouhouOOP/EnemyManager.cpp#L31) | 同一关卡内已有局部时间排程，但时基是毫秒，整关仍由清场波次串接。 |
| 敌人轨迹 | `MoveStrategy` 是 `std::function<void(Enemy*, int)>`，在 `Enemy::move()` 每次更新时调用。[Enemy.h:23](TouhouOOP/Enemy.h#L23) [Enemy.cpp:137](TouhouOOP/Enemy.cpp#L137) | 灵活，但回调不易保存为关卡数据，也不便做确定性回放和编辑器工具。 |
| 弹幕发射 | `bType` 表示发射模式，`BarrageTask` 同时装有模式参数、计时和可变运行状态；`Game::Barrages()` 通过 handler 表派发。[Barrage.h:9](TouhouOOP/Barrage.h#L9) [Role.h:12](TouhouOOP/Role.h#L12) [Game.cpp:608](TouhouOOP/Game.cpp#L608) | 已有可复用的图案分发基础，但配置与运行状态混在同一任务对象里。 |
| 弹幕轨迹 | 每颗 `Barrage` 持有 `BulletMotion`、速度、角度、目标点、原点、半径、阶段等成员；`Barrage::move()` 用 `switch` 更新。[Barrage.h:48](TouhouOOP/Barrage.h#L48) [Barrage.cpp:215](TouhouOOP/Barrage.cpp#L215) | 适合迁移为“紧凑单颗状态 + 轨迹模式处理器”，不需要丢弃现有轨迹种类。 |
| 敌弹存储 | `Barrage::barrList` 是 `vector<Barrage*>`，上限 1800；发射逐颗 `new`，回收逐颗 `delete` 并 `erase`。[Barrage.h:123](TouhouOOP/Barrage.h#L123) [Barrage.cpp:9](TouhouOOP/Barrage.cpp#L9) [Barrage.cpp:93](TouhouOOP/Barrage.cpp#L93) [Barrage.cpp:600](TouhouOOP/Barrage.cpp#L600) | 大量弹幕时每次发射/回收都触及通用堆分配，擦除还会搬动后续指针。 |
| 外部消费者 | AI 和 RL 环境直接拿 `vector<Barrage*>`；RL 会自行生成、删除弹幕。[AIController.h:53](TouhouOOP/AIController.h#L53) [RLEnvironment.cpp:8](TouhouOOP/RLEnvironment.cpp#L8) | 迁移池时必须一并修改消费者接口，否则新池与旧指针列表会形成双重事实来源。 |

## 官作反汇编能支持的结论

分析目标为本机 `th08.exe`（SHA-256 `330FBDBF58A710829D65277B4F312CFBB38D5448B3DF523E79350B879213D924`），使用 REA 6.8.0 和 Ghidra 12.1.4。以下是对该文件的静态反汇编观察，不是原始源码，也不代表已动态测量运行时性能。

### 波次/关卡调度

- 可执行文件引用 `stage1.std` 至 `stage8.std`、分支 `.std` 文件及 `etama.anm`。关卡选择路径加载 `.std`，解析数据头、计数和相对偏移，再建立记录驱动的运行时对象；观察到的场景记录步长为 `0x2A4` 字节。
- 记录类型与处理逻辑关联，场景更新函数遍历运行时记录并执行对象更新。证据支持“外部关卡记录 + 运行时调度/处理器”的结构。
- 目前证据没有完整解出全部记录字段，也没有证明每条记录都是一个敌人波次或精确的时间单位。因此本报告只建议借鉴数据驱动的时间线和事件处理器，不声称复刻 `.std` 格式。

### 弹幕类型、图案与轨迹

- `FUN_00430e10` 按图案配置中的两个维度循环，展开为逐颗发射调用；`FUN_0042f5f0` 从参数计算角度/间距和初始状态，再初始化一颗弹幕实例。
- `FUN_0042f5f0` 中观察到发射模式 `0–8` 的分支。图案负责把参数展开为多个实例，实例随后由普通弹幕运行路径更新。
- 更新器按实例状态字段选择不同处理分支，并更新位置、状态和动画/效果关联数据。实现更接近“紧凑实例状态 + 状态模式处理”，而非给每颗弹分配一段独立的 C++ 多态对象。
- `etama.anm` 与弹种/动画资源加载路径存在关联。弹种图形元数据与每颗活动弹幕状态不是同一概念；本项目也应把视觉数据和运动状态解耦。

### 弹幕槽位和每帧处理

- `FUN_0042f5f0` 从保存的槽位游标开始，最多扫描 `0x600` 个槽，检查槽内状态是否空闲；槽距为 `0x10B8` 字节，找到可用槽后写入活动状态及实例字段。容量上限因此在这条已追踪的普通弹生成路径上是 1536。
- `FUN_00431240` 每帧最多遍历 `0x600` 个槽，空槽跳过，活动槽按状态更新；`FUN_00432170` 将状态字段清零。更新器还单独处理 256 个激光对象，并建立按类型关联的活动对象链表。
- `1536 × 0x10B8 = 6,574,080` 字节，约 6.27 MiB。这只是由槽距和槽数相乘得到的对象区跨度，不等于纯弹幕有效字段大小或运行时私有内存测量。官作槽距很大，不能把 `0x10B8` 当作本项目池元素大小照抄。
- 固定池解决的是重复对象分配和生命周期组织问题；槽位分配仍可能线性扫描。项目可用空闲索引栈做到常数时间分配，同时用活动索引表避免每帧扫描所有空槽。

相关本地证据：`D:\programs\rea-tooling\evidence\th08-stage-handler.json`、`th08-stage-object-update.json`、`th08-etama-emission.json`、`th08-bullet-allocation-confirm.json`、`th08-bullet-processor.json`、`th08-bullet-free.json`。若更换 `th08.exe` 文件，应重新核验哈希并重做对应分析。

## 目标模块和数据边界

### 1. `StageScript`：整关事件时间线

用 `StageDefinition` 保存不可变关卡数据，用 `StageRuntime` 保存当前游标和正在运行的状态。事件用从关卡开始算起的逻辑帧号排序，帧号相同时用 `order` 保持确定执行顺序。

```cpp
enum class StageOp : uint8_t {
    SpawnEnemyGroup,
    StartEmitter,
    StopEmitter,
    SetBossPhase,
    SetBackground,
    ClearBullets,
    WaitForEnemiesClear
};

struct StageEvent {
    uint32_t frame;
    uint16_t order;
    StageOp op;
    StagePayload payload; // C++17 std::variant 或 op + 对应配置结构
};

struct StageRuntime {
    uint32_t frame = 0;
    size_t nextEvent = 0;
};
```

每个模拟帧执行：

1. `while (nextEvent < events.size() && events[nextEvent].frame <= frame)` 取出全部到期事件，按 `order` 执行。
2. `SpawnEnemyGroup` 根据 `count` 和 `periodFrames` 在组内按计划帧生成敌人，不等待上一组死亡。
3. `WaitForEnemiesClear` 是显式屏障；只有脚本确实要求清场时才停住该时间线。普通出场事件不自动等待。
4. 更新完成后推进关卡逻辑帧。渲染帧率和关卡逻辑时钟分离；如果当前主循环只做近似 60 FPS，应先确认并固定模拟步长，不能让每台机器的帧时序决定脚本。

**迁移兼容规则：**旧 `waveData` 暂时通过适配器转换为 `StageEvent`。原先依赖清场推进的 wave 边界，迁移初期转换为显式 `WaitForEnemiesClear`，然后逐段将本应重叠的组改成绝对 `frame`。这样可逐关比较，不会一次改变全部关卡节奏。Boss 配置不要再由待触发事件持有裸 `Boss*`；脚本记录 Boss 类型/阶段配置 ID，到时由工厂创建并由敌人管理器拥有。

### 2. `BulletPattern`、`BulletType` 和 `BulletState`

把当前 `BarrageTask` 拆成只读配置和每个发射器的运行状态：

```cpp
enum class BulletPatternId : uint16_t { Aimed, NWay, Ring, Alternating, CurveFan, OrbitRelease };
enum class BulletTypeId : uint16_t { SmallOrb, Rice, LargeOrb, ... };
enum class BulletMotionId : uint8_t { Linear, CurveThenStraight, BrakeThenAim, Orbit, SineWave, ... };

struct BulletPatternDef {
    BulletPatternId pattern;
    BulletTypeId bulletType;
    BulletMotionId motion;
    uint16_t count;
    float speed;
    float angle;
    float spread;
    uint16_t intervalFrames;
    MotionParams motionParams;
};

struct EmitterRuntime {
    uint32_t nextFireFrame;
    uint32_t shotIndex;
    float angle;
    uint32_t rngState;
};

struct BulletState {
    Vec2 position;
    Vec2 velocity;
    BulletTypeId type;
    BulletMotionId motion;
    uint32_t ageFrames;
    uint16_t flags;
    MotionParams motionState;
};
```

字段的具体类型依游戏需求调整；关键是不要把 `IMAGE`、纹理缓存、关卡配置、发射器计时器和单颗弹轨迹状态塞回同一个对象。

- `BulletPatternDef` 描述“何时发射什么图案、数量/角度/速度参数”，对应现有 `bType` 的几何发射职责。
- `BulletTypeDef` 是只读资源表：贴图区域、缩放、颜色/混合方式、碰撞半径等。把当前 `BulletStyle` 中的渲染和碰撞数据集中到此表；不要让图形类型决定轨迹。
- `BulletMotionId + MotionParams` 表示单颗弹轨迹。把现有 `BulletMotion` 保留并逐个迁入，不必删除 `ORBIT`、`SINE_WAVE` 等已有行为。每帧的 `UpdateBulletMotion(state, fixedDelta)` 只读写 `BulletState`。
- `EmitterRuntime` 保存 `BarrageTask` 中的 `lastTime/currentBurst/currentAngle/jitterState` 等发射器运行数据；`BarrageTask` 剩余静态配置迁入 `BulletPatternDef`。每次触发把 `PatternDef + EmitterRuntime + 发射位置/目标快照` 传入 `EmitPattern()`。
- `EmitPattern()` 计算每颗弹的初始位置、速度、轨迹参数，再向池申请槽位。弹幕生成逻辑不再直接 `new Barrage`。
- 对每一现有模式建立迁移映射，例如 `straightMill/fireWork -> NWay/Ring`、`pincerAim -> AimedPairFan`、`curvedPetal -> CurveFan + CurveThenStraight`。具体映射需用现有玩法参数和轨迹验证，不要仅按函数名映射。

关卡数据先继续使用 C++ 构造器也可以，但应把 lambda handler 改为 `switch(patternId)` 或静态函数表，使定义不捕获 `Game*`，再保存为 JSON/二进制脚本。项目已包含 `json.hpp`；如采用 JSON，第一版建议使用 UTF-8 文本配置并在载入时严格校验：未知枚举、负数量、零间隔、超出容量的 burst 和无效的 Boss ID 均应给出文件名/事件序号错误。

示例（字段名称为建议格式，不是官作文件格式）：

```json
{
  "tickRate": 60,
  "events": [
    {"frame": 0, "op": "spawnGroup", "enemy": "fairyA", "count": 6,
     "periodFrames": 30, "movement": {"id": "sine", "amplitude": 48, "period": 180}},
    {"frame": 90, "op": "startEmitter", "owner": "lastSpawnedGroup",
     "pattern": {"id": "ring", "bulletType": "smallOrb", "count": 12,
                  "speed": 2.2, "intervalFrames": 90}},
    {"frame": 600, "op": "setBossPhase", "boss": "sanae", "phase": 1}
  ]
}
```

如要让运动脚本可序列化，`MoveStrategy` 也应逐步从任意 lambda 改为 `MovementId + MovementParams + MovementRuntime`。初期可保留旧策略作为 `LegacyCallback`，但新关卡数据只能引用登记过的移动 ID。

### 3. `BulletPool`：固定槽位和稳定生命周期

首版敌弹池可以在堆上一次性分配并固定容量；不要把大数组放在栈上。C++17 伪接口：

```cpp
struct BulletHandle {
    uint16_t index;
    uint32_t generation;
};

class BulletPool {
public:
    bool Init(size_t capacity);
    std::optional<BulletHandle> Spawn(const BulletState& initial);
    bool Kill(BulletHandle handle);
    BulletState* TryGet(BulletHandle handle);
    template<class F> void ForEachActive(F&& fn);
    void Update(float fixedDelta);
    void Clear();
};
```

建议内部用三份轻量索引/槽位数据：

- `slots[capacity]`：固定地址的 `BulletSlot { BulletState state; uint32_t generation; bool active; }`。
- `freeIndices`：初始化时装满所有空槽索引，生成时 `pop_back()`，回收时 `push_back()`；分配和回收为 O(1)。
- `activeIndices`：仅列出活动槽，帧更新、碰撞、绘制只遍历活动弹，不需要每帧检查所有空槽。回收时用尾部索引覆盖当前索引并 `pop_back()`，避免 `vector::erase` 搬移。

`generation` 每次回收/复用递增，调用方保存 `{index,generation}`，旧句柄即使索引复用也不能读写新弹。发射时池满则显式返回失败并累计 `poolFullDrops`，不覆盖仍活动槽；这样不会随机改变现存弹轨迹。统计 `peakActive`、满池次数和分配次数。

容量决策分两步：迁移期先设 2048 槽，覆盖当前 1800 的已声明上限并留少量边界；加入每关 `peakActive` 记录及满池测试后，再判断 1536 是否能覆盖既有关卡。如果必须限制为官作观察到的 1536，则要明确设计超限行为并证明弹幕玩法可接受。不要把官作的 `0x10B8` 字节槽距照搬；项目新 `BulletState` 应只保留仿真必需字段，视觉资源由 `BulletTypeDef` 共享。

**与现有消费者的接合：**

1. 将 `Barrage` 从“管理器+单颗弹幕”拆开，新增 `BulletPool`/`BulletManager`。先保留一个薄的 `Barrage` facade，内部调用 pool，方便分批替换发射函数。
2. 将 `Game::CheckCollision()`、绘制、弹幕清场和计数改为 manager 的只读活动迭代器/API。
3. 把 `AIController` 的 `vector<Barrage*>` 入参改成 `const BulletPool&` 或 `BulletReadView`，避障只读取活动弹坐标。
4. 把 `RLEnvironment` 的裸 `vector<Barrage*>*` 换成 `BulletPool*`，训练弹也通过同一个 `Spawn()` API 创建；环境重置只调用 `Clear()`，不得自行 `delete` 槽中对象。
5. 玩家弹 `Bullet`、符卡弹幕和特效可先保持各自管理。敌弹池稳定后，再评估它们是否共享同一种槽位系统，避免一次扩大改造范围。

## 逐文件改造清单

| 文件 | 修改内容 |
|---|---|
| 新增 `StageScript.h/.cpp` | 定义 `StageDefinition/StageEvent/StageRuntime`、事件载荷、稳定排序、脚本校验、逻辑帧推进和事件执行。关卡数据建议先放 `resource/stage/`，不把二进制 `.std` 当成格式要求。 |
| `Game.h/.cpp` | 将 `waveQueue/currentWave/nextWave/wait/waitStart` 逐步改为 `StageRuntime`；`HandleRound()` 只推进阶段事件、更新系统，不再隐式用 `checkEnemyClear()` 决定所有后续事件；添加 `WaitForEnemiesClear` 兼容动作。普通关与 AI 演示可共用脚本，RL 训练保持自己的 reset 流程。 |
| `EnemyManager.h/.cpp` | 将 `eventQueue/activeEvents/waveStartTime` 迁出调度器；保留敌人实例生命周期和 spawn API。用 `EnemyDefinitionId`/配置值创建敌人，待触发关卡事件不再拥有 `Boss*`；将 `forceExitExisting` 替换为清晰的离场命令。 |
| `Role.h`、`Enemy.h/.cpp`、`Movement.h` | 把 `BarrageTask` 拆为只读发射配置与 `EmitterRuntime`；把 `MoveStrategy` 可选地迁为 `MovementId + Params + Runtime`。旧 lambda 暂时保留为 legacy adapter，直到所有关卡数据迁移完。 |
| 新增 `BulletTypes.h`、`BulletPattern.h/.cpp` | 定义 `BulletTypeId/BulletTypeDef`、`BulletPatternId/BulletPatternDef`、`BulletMotionId/MotionParams` 和 `EmitPattern()`。发射计算不访问 `barrList`，只通过池的 spawn API 创建初始状态。 |
| 新增 `BulletPool.h/.cpp` | 管理固定 `BulletState` 槽、空闲索引、活动索引、generation 句柄、池满统计、更新和清场。池元素不持有贴图/`IMAGE`，绘制器用弹种 ID 查共享资源。 |
| `Barrage.h/.cpp` | 迁移期间保留 facade；把 `barrList` 的拥有权和 `Barrage` 单颗对象逐步移除。先迁移 `Normal/straightMill/fireWork/circleMill`，再迁移曲线/轨道等特殊轨迹，最后移除所有逐弹 `new/delete`。 |
| `Game.cpp` 的碰撞/渲染调用点 | 将遍历 `Barr.barrList` 改为 `BulletManager` 的活动只读迭代；命中或出界时调用 `Kill(handle)`/标记回收，不直接 delete，也不在活动遍历中使迭代器失效。 |
| `AIController.h/.cpp` | 把 `const std::vector<Barrage*>&` 改为 `BulletReadView` 或 `const BulletPool&`；保留现有 AI 状态 JSON 字段，只改变取数据的适配层。 |
| `RLEnvironment.h/.cpp` | 把 `std::vector<Barrage*>*` 改为池接口；训练弹使用与实战相同的 spawn、move、kill 生命周期。`ClearBullets()` 只调用池清空，不自行释放槽位内存。 |
| `TouhouOOP.vcxproj`、`.filters` | 新增的 `.cpp/.h` 加入所有目标配置与筛选器；保持项目 C++17。若使用 JSON 配置，解析器复用已有 `json.hpp`，不用再引入第二套 JSON 库。 |

推荐按“一种图案、一种轨迹、一关脚本”的切片迁移，每个切片都编译和运行后再拓展。这样出现偏差时可以定位在事件时间、发射展开、轨迹更新或槽位回收中的一个边界。

## 推荐实施顺序

### 阶段 0：行为基线与指标

- 为当前每关记录敌弹峰值、每秒生成/回收数量、堆分配数量、波次开始时刻和死亡时刻。
- 固定随机数种子；保存关键弹幕的逐帧位置样本或测试关卡录像，作为轨迹回归基线。
- 给 `Game::HandleRound()` 的现有 clear-gate 行为加日志/测试入口，特别覆盖 Boss 与 `forceExitExisting`。

**完成条件：**可以回答各关最高活动弹数、1800 上限是否触顶，以及哪些 wave 确实需要清场屏障。

### 阶段 1：改成关卡时间线，不立即改敌人与轨迹

- 新建 `StageScript.h/.cpp`，定义事件、事件载荷校验、稳定排序和 `StageRuntime::Tick()`。
- 为旧 `waveQueue` 写转换器；按旧节奏插入 `WaitForEnemiesClear`，确认脚本与旧版本的刷怪和 Boss 次序一致。
- 选一个普通关卡，把原本应该重叠的波次改为绝对帧事件；确认仍在场敌人不阻塞这些事件。
- 将 `forceExitExisting` 改为显式 `ExitEnemies`/`RetreatEnemies` 事件，不把无实体动作伪装成 count 为 0 的刷怪组。
- Boss 使用定义 ID，由事件触发时创建；关卡定义不持有裸指针。

**完成条件：**同一个事件脚本在敌人被打死、未被打死两种情况下，除显式屏障外都在同一逻辑帧触发。

### 阶段 2：分离弹幕数据与逐弹轨迹

- 新增 `BulletPatternDef`、`EmitterRuntime`、`BulletTypeDef`、`BulletState` 和 `MotionParams`。
- 先迁移 `Normal/down_st`、环形发射、N-way 等基础模式，再迁移正弦、曲线、刹停瞄准、绕转释放等状态模式。
- 用静态 pattern dispatch 替换 `unordered_map<bType, std::function<...>>` 对发射模式的闭包派发；保留现有任务时间行为直到固定帧调度可用。
- 分离渲染与仿真：`BulletState` 不保存 `IMAGE`。渲染器按 `BulletTypeId` 查共享纹理/尺寸；碰撞半径也由弹种定义表提供。
- 将轨迹从毫秒 `GetTickCount()` 依赖迁至 `ageFrames`/逻辑帧，发射冷却使用 `nextFireFrame`。如果采用固定 60 Hz，毫秒配置按 `round(ms * 60 / 1000)` 转帧并在配置里标明舍入。

**完成条件：**固定 seed 下，同配置的图案每次生成相同初始弹组；轨迹测试覆盖每种 `BulletMotionId` 的分段时刻、释放时刻和边界条件。

### 阶段 3：用固定池替换敌弹拥有权

- 实现池和句柄单元测试：容量 0/1、满池、回收后复用、旧 generation 句柄失效、同帧多个回收、clear 后重新 spawn。
- 将所有敌弹发射入口改成 `BulletPool::Spawn()`；任何图案函数不得再 `new Barrage`。
- 将移动、碰撞和绘制循环移入 manager；更新时禁止在活动迭代中直接做会使迭代器失效的删除。
- 接通碰撞、AI 状态导出、RL 训练、符卡/清场代码，逐一移除对 `barrList` 的拥有权操作。
- 加入 `poolFullDrops`、`peakActive` 和 `activeCount` 调试指标；确认清场/死亡/换关不会留下槽位活动标志。

**完成条件：**敌弹发射/回收热点没有逐弹 `new/delete`；压力测试中活动数从 0 到池容量、再降回 0 可循环多次，无泄漏、重复回收或旧句柄误指。

### 阶段 4：数据化关卡并收敛旧接口

- 把选定关卡的 C++ 波次构造代码迁为 `stageN.json` 或等价文本配置；从一关开始，逐步扩大。
- 迁移 `MoveStrategy` lambda 到已登记 `MovementId`，保留少量明确标记的 legacy 路径直到旧关卡完成迁移。
- 删除 `waveQueue/nextWave` 的普通流程依赖；保留清场语义为脚本事件，而非隐含在 `HandleRound()`。
- 删除 `vector<Barrage*>` 兼容 facade 和直接删除操作。

**完成条件：**改关卡出场/发射节奏只需改关卡数据；脚本解析失败能定位文件与事件；普通模式、AI 演示和 RL 训练都使用同一个敌弹池生命周期接口。

## 测试与验收清单

- **时间线**：同帧多事件按 `order` 稳定执行；事件遗漏/重复、零间隔、帧回绕、暂停后恢复有明确测试；死敌不影响绝对时间事件。
- **旧玩法兼容**：固定 seed 下比较首波、每个 Boss 阶段、弹幕发射帧与关键弹位置；刻意改为并行的事件单独做预期变更记录。
- **轨迹**：每个 motion 至少覆盖出生帧、模式切换帧、释放后数帧、出界与死亡；测试不同更新帧率时固定模拟步长结果相同。
- **池生命周期**：溢出返回可观察结果、不覆盖活动弹；重复 kill 安全失败；旧句柄在槽复用后失效；`Clear()` 后 active/free 计数正确。
- **性能**：用代表性高弹关卡测每帧更新时间、碰撞/绘制耗时、峰值活动弹；确认池容量留有缓冲。性能结论以本项目测量为准，不以“官作 1536”推算。
- **集成**：AI 避障遍历活动只读视图；RL 训练 bullet 通过 pool spawn；关卡重置和游戏退出清空 manager；Boss/符卡清场不会留下悬空句柄。

## 需要避免的误区

1. 不要直接仿造 `.std` 字节格式。我们尚未完整解码所有记录字段；自定义清晰的脚本格式更易维护。
2. 不要把 `0x10B8` 当成单颗弹幕的推荐 C++ 结构体大小。它是本二进制某槽位的步长，含有当前分析未逐字段命名的数据。
3. 不要把 pool capacity 从 1800 直接改成 1536。先记录峰值、触顶次数和超限图案；确定拒绝策略后再决定目标容量。
4. 不要把“弹幕类型”混为一谈：`BulletPatternId` 是发射形状，`BulletTypeId` 是贴图/碰撞定义，`BulletMotionId` 是单颗轨迹。
5. 不要让脚本事件持有 `Enemy*`/`Boss*` 所有权，也不要让 AI/RL 保留池中对象的裸拥有指针。
6. 不要同时混用墙钟毫秒、每帧速度和帧号却没有转换约定。先确定一个固定逻辑时钟，再迁移发射与轨迹计时。

## 建议的首个可交付切片

以一段 10–20 秒的普通敌人关卡为试点：把两个 `SpawnEvent` 组转成带绝对帧号的 `SpawnEnemyGroup`，加一个显式清场屏障用于比较旧节奏；将一个 `firework` 发射器拆成 `Ring` pattern、一个 `BulletTypeDef` 和 `Linear` `BulletState`；将该关的敌弹接入 2048 槽池，但暂时保留原渲染函数作为池的消费者。对比旧/新版本的生成帧、弹幕位置、`peakActive`、池满次数和每帧耗时。此切片跑稳后，再扩展曲线轨迹、Boss 和其余关卡。
