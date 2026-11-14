#pragma once

#include "EnemyManager.h"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

enum class StageEventType {
	SpawnGroup,             // 在指定时间启动一个刷怪组
	RetreatNonBossEnemies,  // 让现有非 Boss 敌人开始离场
	WaitForEnemiesClear,    // 等待敌人和待生成敌人组清空
	Callback                // 执行自定义关卡动作，例如生成 Boss
};

struct StageEvent {
	std::uint64_t frame = 0;          // 事件触发的关卡逻辑帧
	int order = 0;                    // 同一帧内的优先顺序，数值较小的先执行
	StageEventType type = StageEventType::Callback; // 事件类型
	SpawnGroupDef spawnGroup;         // SpawnGroup 事件的刷怪配置
	std::function<void()> callback;   // Callback 事件要执行的动作
};

class StageTimeline {
private:
    std::vector<StageEvent> events;   // 已排序的关卡事件列表
    std::size_t nextEvent = 0;        // 下一项待处理事件的下标
    std::uint64_t frame = 0;          // 当前关卡脚本时间，清场等待时暂停

public:
    void Load(std::vector<StageEvent> stageEvents) {
        // 先按触发帧排序；同帧再按 order 排序。
        // stable_sort 会保留 frame 和 order 都相同的事件原始添加顺序。
        std::stable_sort(stageEvents.begin(), stageEvents.end(),
            [](const StageEvent& left, const StageEvent& right) {
                if (left.frame != right.frame) {
                    return left.frame < right.frame;
                }
                return left.order < right.order;
            });

        events = std::move(stageEvents);
        nextEvent = 0; // 从第一项事件开始
        frame = 0;     // 新关卡从第 0 帧计时
    }

    void Clear() {
        events.clear(); // 删除当前关卡脚本
        nextEvent = 0;
        frame = 0;
    }

    void Tick(
        const std::function<bool()>& enemiesClear, // 查询敌人是否清空
        const std::function<void(const StageEvent&)>& dispatch) { // 派发普通事件
        while (nextEvent < events.size()
            && events[nextEvent].frame <= frame) {
            const StageEvent& event = events[nextEvent];

            if (event.type == StageEventType::WaitForEnemiesClear) {
                // 未清场时不消费这个事件，也不推进脚本 frame。
                // 下一游戏帧会再次检查。
                if (!enemiesClear()) return;
            } else {
                // 生成敌人、令敌人离场、执行回调都由 Game 派发。
                dispatch(event);
            }

            ++nextEvent; // 当前事件处理完成
        }

        ++frame; // 没被清场门阻塞时，关卡脚本前进一帧
    }
};