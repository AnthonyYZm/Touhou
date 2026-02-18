#pragma once
#include "Role.h"
#include "Bullet.h"
#include <map>
#include <functional>

enum class eType : int {
	normal = 0,
	elf = 1,
	boss = 2
};

class Enemy;	

// 定义移动策略函数签名：接收 自身指针 和 存活时间(ms)
using MoveStrategy = std::function<void(Enemy*, int)>;

/// <summary>
/// @brief Enemy class
/// @details Inherited from Role class, represents enemy characters
/// </summary>
class Enemy : public Role {

	static const int normalWidth;
	static const int normalHeight;
	static const int elfWidth;
	static const int elfHeight;
	static const int bossWidth;
	static const int bossHeight;
	static IMAGE img;
	static IMAGE sanae;
	static bool resLoaded;
	int actionState;
	
	DWORD te1, te2;
	IMAGE enemy1, Elf;	
	static IMAGE imgSpellCircle;
	float spellAngle = 0.0f;

protected:

	float enemyX;
	float enemyY;
	float vx, vy;

public:

	MoveStrategy mover; 
	DWORD birthTime; 
	eType type;
	static const int bossFrameW = 64; 
	static const int bossFrameH = 85; 
	int texWidth;
	int texHeight;
	int currentPhaseIdx;
	int moveCount;
	Enemy(float _x = 0, float _y = 0, int _hp = 1);

	int col, frame, row, sx, sy;
	int width, height;
	int maxHp;
	int phase;
	
	static std::map<eType, float> speedMap; 
	std::vector<BarrageTask> tasks;

	void draw() override;
	virtual void move() override;
	void InitRound();
	bool checkEnemyClear();
	bool isFire() const { return fire; }
	bool isAlive() const { return alive; }
	void setStrategy(MoveStrategy ms) { mover = ms; }	

	void AddTask(BarrageTask task) { tasks.push_back(task); }
	void ClearTasks() {	tasks.clear(); }
	std::vector<BarrageTask>& GetTasks() { return tasks; }

	friend int getEnemyWidth(Enemy& e);
	friend int getEnemyHeight(Enemy& e);
	float getEnemyX() const { return enemyX; }

	void setVx(float _vx) { vx = _vx; };	
	void setVy(float _vy) { vy = _vy; };
	void setAction(int state) { actionState = state; }
};

