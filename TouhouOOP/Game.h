#pragma once
#include "Role.h"
#include "Hero.h"	
#include "Bullet.h"
#include "Screen.h"
#include "Enemy.h"
#include "Barrage.h"
#include "EnemyManager.h"
#include <queue>


// Enemy config
struct enemyData {
	eType type;
	bType barrageType;
	int interval;   // 敌人刷新的时间间隔
	int waveDelay; // 开始刷新的延迟时间
	float barrSpeed;
	std::vector<float> position;
	float param[10] = { 0 }; // 预留参数
};

class Game {

	static int bulletLevel;
	bool enemyFire;
	bool wait;
	Screen Scr;
	EnemyManager E;
	Hero Hero;
	Barrage Barr;	
	Bullet B;

	std::queue<enemyData> waveQueue; // 使用队列管理波次
	enemyData wave;

	DWORD waitStart;

public:
	Game();
	~Game();

	void Touhou();
	void InitLevels();
	void HandleRound();

	void Bullets();	
	void HeroControl();	

	void Barrages();
	void Enemies();
};

