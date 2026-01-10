#pragma once
#include "Role.h"
#include "Bullet.h"
#include <map>

/// <summary>
/// @brief Enemy class
/// @details Inherited from Role class, represents enemy characters
/// </summary>
class Enemy : public Role {

	unsigned int type; //round enemy type
	int col, frame, row;
	float enemyX;
	float enemyY;

	static const int normalWidth;
	static const int normalHeight;
	static const int elfWidth;
	static const int elfHeight;

	DWORD te1, te2;
	IMAGE enemy1, Elf;	

public:

	Enemy(float _x = 0, float _y = 0, int _hp = 1);

	static std::map<eType, float> speedMap; 

	void draw() override;
	void draw2();	
	void createEnemy(const eType& type, std::vector<Enemy*>& list);
	void move(const eType& type, std::vector<Enemy*>& list, float speed);
	void InitRound();
	bool checkEnemyClear();
	bool collision(const eType& type, Bullet* bullet, std::vector<Enemy*>& list);
	bool isFire() const { return fire; }
	bool isAlive() const { return alive; }

	void EnemyX(); //set round's enemyX
	void EnemyNum(); //set round's enemyNum
	void resetTimers();
	// count: 这一波怪的总数
	// interval: 出怪间隔


	static int getNormalWidth() { return normalWidth; }
	static int getNormalHeight() { return normalHeight; }
	static int getElfWidth() { return elfWidth; }
	static int getElfHeight() { return elfHeight; }
	float getEnemyX() const { return enemyX; }

};

