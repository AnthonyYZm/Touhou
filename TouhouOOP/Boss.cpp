#include "Boss.h"

Boss::Boss(float _x, float _y) : Enemy(_x, _y, 1000) {
    currentPhaseIdx = -1;
    phaseStartTime = GetTickCount();
    tasksLoaded = false;
    type = eType::elf; 
    frame = 5; 
}

void Boss::addPhase(BossPhase p) {
    phases.push_back(p);
    if (phases.size() == 1) {
        currentPhaseIdx = 0;
        hp = p.phaseHp;
        maxHp = p.phaseHp;
    }
}

void Boss::loadCurrentPhase() {
    if (currentPhaseIdx >= phases.size()) return;
    BossPhase& p = phases[currentPhaseIdx];
    // 重置状态
    phaseStartTime = GetTickCount();
    this->ClearTasks(); 
    // 加载新弹幕
    for (auto& task : p.tasks) {
        this->AddTask(task);
    }
    // 重置血量
    this->maxHp = p.phaseHp;
    this->hp = p.phaseHp;
    // 触发特效
    if (onEvent) {
        if (p.isSpellCard) {
            onEvent(x, y, 1); 
        }
        else {
        }
    }
    tasksLoaded = true;
}

void Boss::nextPhase() {
    if (onEvent) onEvent(x, y, 2);
    currentPhaseIdx++;
    tasksLoaded = false; 
    if (currentPhaseIdx >= phases.size()) {
        alive = false; 
    }
}

void Boss::move() {
    if (!alive) return;
    if (phases.empty()) return;
    if (currentPhaseIdx == -1) currentPhaseIdx = 0;
    if (!tasksLoaded) {
        loadCurrentPhase();
        if (!alive) return; 
    }
    BossPhase& currentP = phases[currentPhaseIdx];
    DWORD now = GetTickCount();
    int timePassed = now - phaseStartTime;
    int t = now - phaseStartTime;
    if (t < currentP.waitTime) this->fire = false; 
    else this->fire = true;  
    if (currentP.moveLogic){ 
        currentP.moveLogic(this, timePassed);
    }
    // 条件 A: 击破
    // 条件 B: 超时
    if (hp <= 0 || timePassed >= currentP.duration) {
        nextPhase();
    }
}

bool Boss::isSpellCardState() const {
    if (currentPhaseIdx >= 0 && currentPhaseIdx < phases.size())
        return phases[currentPhaseIdx].isSpellCard;
    return false;
}

int Boss::getPhaseMaxHp() const {
    if (currentPhaseIdx >= 0 && currentPhaseIdx < phases.size())
        return phases[currentPhaseIdx].phaseHp;
    return 1;
}

int Boss::getSpellID() const {
    if (currentPhaseIdx >= 0 && currentPhaseIdx < phases.size())
        return phases[currentPhaseIdx].spellID;
    return -1;
}

int Boss::getPhaseTimeLeft() const {
    if (currentPhaseIdx >= 0 && currentPhaseIdx < phases.size()) {
        int passed = GetTickCount() - phaseStartTime;
        int left = phases[currentPhaseIdx].duration - passed;
        return (left > 0) ? (left / 1000) : 0; 
    }
    return 0;
}