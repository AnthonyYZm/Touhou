#pragma once
#include "Enemy.h"
#include <vector>

// Boss 的阶段配置
struct BossPhase {
    int duration;           // 持续毫秒
    MoveStrategy moveLogic; // 该阶段怎么动
    std::vector<BarrageTask> tasks; // 该阶段要发射什么弹幕
    int spellID; // 阶段符卡
};

class Boss : public Enemy {
private:
    std::vector<BossPhase> phases;
    int currentPhaseIdx;
    DWORD phaseStartTime;
    bool tasksLoaded; // 标记当前阶段任务是否已加载

public:
    // 特效回调接口：让 Game 层去实现具体的符卡播放
    std::function<void(float x, float y, int effectID)> spell;

    Boss(float _x, float _y, int _hp);

    void addPhase(BossPhase p);
    void move() override; // 覆盖基类
};