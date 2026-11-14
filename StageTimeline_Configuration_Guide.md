# 关卡时间线配置指南

本次改造把普通关和 AI 演示关的出场时间放进 `StageTimeline`。关卡仍用 C++ 配置，因为移动策略是 `MoveStrategy` lambda，不能直接序列化成 JSON。每帧先派发到期的关卡事件，再更新刷怪组、敌人和弹幕。

## 修改位置

- [StageTimeline.h](TouhouOOP/StageTimeline.h)：事件类型、按帧排序和清场等待。
- [EnemyManager.h](TouhouOOP/EnemyManager.h)：静态刷怪组定义与运行时刷怪状态分离。
- [EnemyManager.cpp](TouhouOOP/EnemyManager.cpp)：刷怪间隔按 60Hz 逻辑帧推进。
- [Game.cpp](TouhouOOP/Game.cpp)：普通关、AI 演示关、Boss 和关卡事件的配置与派发。
- [Game.h](TouhouOOP/Game.h)：持有时间线和世界帧计数。

## 时间单位

关卡事件的 `frame` 是从进入模式开始计算的逻辑帧。游戏目标帧率是 60Hz，`GameFramesFromMilliseconds(ms)` 用向上取整换算：1 秒约 60 帧，10 秒约 600 帧。刷怪组的 `intervalMs` 也按这个规则转换，因此 520ms 会取 32 帧，约 533ms。

普通关的 `MakeStageSpawn(timeMs, ...)` 保留原来的关卡相对时间，并自动加上 1000ms 开场等待。因此 `MakeStageSpawn(10000, ...)` 会在进入普通模式约 11 秒时触发。退场事件显式设在模式开始后的 117 秒，对应开场 1 秒加原来关卡中的 116 秒。

## 新增一组敌人

在 `Game.cpp` 的 `Game::InitStageDemo(std::vector<StageEvent>& events)` 中，照以下形式新增事件：

```cpp
events.push_back(MakeStageSpawn(
    10000,                  // 普通关计时 10 秒，实际含 1 秒开场等待
    6,                      // 数量
    500,                    // 每只间隔 500ms
    eType::normal,          // normal / elf
    8,                      // HP
    (float)LeftEdge + 80.0f,
    (float)TopEdge - 40.0f,
    Moves::ZigzagDive(
        (float)LeftEdge + 100.0f,
        (float)TopEdge - 40.0f,
        70.0f,               // 横向摆幅
        1500,                // 转向节奏参数
        220.0f,              // 下行距离
        0.0f                 // 初始相位
    )
));
```

函数参数顺序是：关卡时间毫秒、数量、生成间隔毫秒、敌人类型、生命值、初始 X/Y、移动策略、可选的初始弹幕任务。每个生成组只描述静态配置；已刷数量和下次刷出帧保存在 `EnemyManager::PendingSpawnGroup` 中。

若希望敌人出生时携带弹幕任务，最后增加 `BarrageTask` 列表：

```cpp
const std::vector<BarrageTask> tasks = {
    BarrageTask((int)bType::down_st, 1800, 2.0f, 0.0f, 1,
        0, 1, 0, 0, 0.0f, 1, 0, (int)BulletStyle::GREEN_)
};

events.push_back(MakeStageSpawn(
    12000, 1, 0, eType::elf, 24,
    (float)Right - 100.0f, (float)TopEdge,
    Moves::EnterHoldExit(
        { (float)Right - 100.0f, (float)TopEdge },
        { (float)Right - 160.0f, (float)TopEdge + 180.0f },
        1500, 7000, 185.0f),
    tasks));
```

弹幕类型、速度、方向、发射数量、颜色和移动策略继续使用现有 `BarrageTask`、`bType`、`BulletStyle` 和 `Moves::*` 配置。

## 配置退场、清场门和 Boss

关卡控制事件不伪装成敌人生成组。当前普通关在模式开始后 117 秒安排退场，然后等待所有敌人和未完成生成组清空：

```cpp
StageEvent retreat;
retreat.frame = GameFramesFromMilliseconds(117000);
retreat.type = StageEventType::RetreatNonBossEnemies;
events.push_back(retreat);

StageEvent waitForClear;
waitForClear.frame = retreat.frame;
waitForClear.order = 1;
waitForClear.type = StageEventType::WaitForEnemiesClear;
events.push_back(waitForClear);
```

Boss 事件安排在清场门之后 2500ms。清场等待期间时间线帧暂停，但 `worldFrame` 和敌人更新继续前进；敌人离场后脚本计时恢复：

```cpp
AddSanaeBossEvent(events, false,
    GameFramesFromMilliseconds(117000 + 2500));
```

同一帧的事件先比较 `order`，相同 `order` 则保持添加顺序。想让某个回调改变背景、播放剧情或触发其他关卡动作，可以添加 `StageEventType::Callback`，并给 `callback` 设置 lambda。

## 选择关卡时间线

- 普通模式由 `InitNormalLevels()` 创建普通关事件列表并加载到 `stageTimeline`。
- AI 演示由 `InitAILevel()` 加载 Boss 演示事件，Boss 在模式开始 2.5 秒后出场。
- RL 训练不加载普通关时间线，由 Python 的 `reset/step` 驱动训练场景。

要创建新的普通关配置，可以复制 `InitStageDemo` 的事件构造函数，在新函数中填充 `std::vector<StageEvent>`，再由对应模式初始化函数调用 `stageTimeline.Load(std::move(events))`。切换或重开模式前先调用 `stageTimeline.Clear()`、`E.clearEnemy()` 并将 `worldFrame` 设为 0。

## 运行时注意点

- `WaitForEnemiesClear` 会等待敌人列表和待刷组都为空；如果清场门前还有尚未刷完的组，该组仍会继续生成并最终结束。
- 生成间隔为 0 时，多只敌人仍按每帧最多一只生成，避免同一帧瞬间创建整组对象。
- 时间线以逻辑帧计时；程序掉帧时，时间线随实际逻辑更新次数变慢，不会通过一次大步进跳过大量事件。
- 移动轨迹参数仍以现有 `Moves::*` 实现为准，移动策略计时仍使用敌人出生后的毫秒数。
