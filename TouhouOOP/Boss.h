#pragma once
#include "Enemy.h"
#include <vector>

// Boss 的阶段配置
struct BossPhase {
    int duration;           // 持续毫秒
    MoveStrategy moveLogic; // 该阶段怎么动
    std::vector<BarrageTask> tasks; // 该阶段弹幕
    int spellID; 
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
    bool tasksLoaded; 
    void loadCurrentPhase();
    void nextPhase();

public:
    
    std::function<void(float x, float y, int effectID)> onEvent;

    Boss(float _x, float _y);

    void addPhase(BossPhase p);
    void move() override; 

    bool isSpellCardState() const;
    int getSpellID() const;
    float getTimeLeftRate() const; 
    int getPhaseMaxHp() const;
    int getPhaseTimeLeft() const;
};