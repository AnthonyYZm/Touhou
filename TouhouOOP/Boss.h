#pragma once
#include "Enemy.h"
#include <vector>

// Boss 的阶段配置
struct BossPhase {
    int duration;           // 持续毫秒
    MoveStrategy moveLogic; // 该阶段怎么动
    std::vector<BarrageTask> tasks; // 该阶段要发射什么弹幕
    int spellID; // 阶段符卡
    bool isSpellCard;
    int phaseHp;
    int waitTime;

    BossPhase(int _hp, int _time, MoveStrategy _move, bool _isSpell = false, int _spellID = 0, int _wait = 2000)
        : phaseHp(_hp), duration(_time), moveLogic(_move), isSpellCard(_isSpell), spellID(_spellID), waitTime(_wait) {
    }
};

class Boss : public Enemy {
private:
    std::vector<BossPhase> phases;
    int currentPhaseIdx;
    DWORD phaseStartTime;
    bool tasksLoaded; // 标记当前阶段任务是否已加载

    void loadCurrentPhase();
    void nextPhase();

public:
    // 特效回调接口：让 Game 层去实现具体的符卡播放
    std::function<void(float x, float y, int effectID)> onEvent;

    Boss(float _x, float _y);

    void addPhase(BossPhase p);
    void move() override; // 覆盖基类

    bool isSpellCardState() const;
    int getSpellID() const;
    float getTimeLeftRate() const; // 返回 0.0 - 1.0 用于画倒计时圆环
    int getPhaseMaxHp() const;
    int getPhaseTimeLeft() const;
};