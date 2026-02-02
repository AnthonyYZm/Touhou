#include "Boss.h"

Boss::Boss(float _x, float _y) : Enemy(_x, _y, 1000) {
    currentPhaseIdx = -1;
    phaseStartTime = GetTickCount();
    tasksLoaded = false;
    type = eType::elf; // 使用 Elf 的贴图，或者定义新的 Boss 贴图
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

    // 1. 重置状态
    phaseStartTime = GetTickCount();
    this->ClearTasks(); // 清空上一阶段弹幕

    // 2. 加载新弹幕
    for (auto& task : p.tasks) {
        this->AddTask(task);
    }

    // 3. 重置血量
    this->maxHp = p.phaseHp;
    this->hp = p.phaseHp;

    // 4. 触发特效回调
    if (onEvent) {
        if (p.isSpellCard) {
            onEvent(x, y, 1); // ID 1: 符卡展开特效 (Cut-in, 背景变色)
        }
        else {
            // 非符开始通常不需要大特效，或者只需要简单的提示
        }
    }

    tasksLoaded = true;
}

void Boss::nextPhase() {
    // 阶段结束特效
    if (onEvent) onEvent(x, y, 2); // ID 2: 阶段击破爆炸

    currentPhaseIdx++;
    tasksLoaded = false; // 标记下帧加载

    // 如果所有阶段都打完了
    if (currentPhaseIdx >= phases.size()) {
        alive = false; // Boss 死亡
    }
}

void Boss::move() {
    if (!alive) return;
    if (phases.empty()) return;
    if (currentPhaseIdx == -1) currentPhaseIdx = 0;

    // 1. 加载阶段 (如果是新阶段的第一帧)
    if (!tasksLoaded) {
        loadCurrentPhase();
        if (!alive) return; // 如果已经没阶段了
    }

    BossPhase& currentP = phases[currentPhaseIdx];
    DWORD now = GetTickCount();
    int timePassed = now - phaseStartTime;
    int t = now - phaseStartTime;
    if (t < currentP.waitTime) this->fire = false; 
    else this->fire = true;  
    // 2. 执行移动策略
    if (currentP.moveLogic){ 
        currentP.moveLogic(this, timePassed);
    }

    // 3. 阶段转换判定
    // 条件 A: 血量归零 (击破)
    // 条件 B: 时间结束 (超时)
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
        return (left > 0) ? (left / 1000) : 0; // 转换为秒
    }
    return 0;
}