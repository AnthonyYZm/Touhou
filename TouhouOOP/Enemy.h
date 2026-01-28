#pragma once
#include "Role.h"
#include "Bullet.h"
#include <map>
#include <functional>

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

	DWORD te1, te2;
	IMAGE enemy1, Elf;	

protected:

	float enemyX;
	float enemyY;
	float vx, vy;

public:

	MoveStrategy mover; 
	DWORD birthTime; 

	Enemy(float _x = 0, float _y = 0, int _hp = 1);

	int col, frame, row, sx, sy;
	int width, height;
	
	static std::map<eType, float> speedMap; 
	std::vector<BarrageTask> tasks;

	void draw() override;
	virtual void move() override;
	void draw2();	
	void InitRound();
	bool checkEnemyClear();
	bool isFire() const { return fire; }
	bool isAlive() const { return alive; }
	void setStrategy(MoveStrategy ms) { mover = ms; }	
	void EnemyX(); //set round's enemyX

	// 添加任务
	void AddTask(BarrageTask task) { tasks.push_back(task); }
	void ClearTasks() {	tasks.clear(); }
	std::vector<BarrageTask>& GetTasks() { return tasks; }

	static int getNormalWidth() { return normalWidth; }
	static int getNormalHeight() { return normalHeight; }
	static int getElfWidth() { return elfWidth; }
	static int getElfHeight() { return elfHeight; }
	float getEnemyX() const { return enemyX; }

	void setVx(float _vx) { vx = _vx; };	
	void setVy(float _vy) { vy = _vy; };

};

