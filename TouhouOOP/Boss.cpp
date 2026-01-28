#include "Boss.h"

Boss::Boss(float _x, float _y, int _hp) : Enemy(_x, _y, _hp) {
    currentPhaseIdx = 0;
    phaseStartTime = GetTickCount();
    tasksLoaded = false;
    type = eType::elf; // 使用 Elf 的贴图，或者定义新的 Boss 贴图
    frame = 5; // 精灵贴图帧数
}

void Boss::addPhase(BossPhase p) {
    phases.push_back(p);
}

void Boss::move() {
    if (!alive || phases.empty()) return;

    // 如果所有阶段都跑完了，保持最后一个动作或循环，这里设为不动
    if (currentPhaseIdx >= phases.size()) return;

    BossPhase& currentP = phases[currentPhaseIdx];
    DWORD now = GetTickCount();

    // 1. 初始化当前阶段的任务 (只在进入阶段的第一帧执行)
    if (!tasksLoaded) {
        phaseStartTime = now;
        this->ClearTasks();
        for (auto& task : currentP.tasks) {
            this->AddTask(task);
        }
        tasksLoaded = true;
    }

    int t = now - phaseStartTime;

    // 2. 执行移动
    if (currentP.moveLogic) {
        currentP.moveLogic(this, t);
    }

    // 3. 检查阶段结束
    if (t >= currentP.duration) {
        // 触发特效
        if (currentP.spellID > 0 && spell) {
            spell(x, y, currentP.spellID);
        }

        // 切换
        currentPhaseIdx++;
        tasksLoaded = false; // 标记需要在下一帧加载新任务
    }
}