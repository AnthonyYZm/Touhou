#pragma once
#include "Role.h"
#include "Hero.h"	
#include "Bullet.h"
#include "Screen.h"
#include "Enemy.h"
#include "Barrage.h"
#include <queue>


// Enemy config
struct enemyData {
	eType type;
	bType barrageType;
	int enemyCount; // 一波刷多少敌人
	int interval;   // 敌人刷新的时间间隔
	int waveDelay; // 开始刷新的延迟时间
};

class Game {

	static int bulletLevel;
	bool enemyFire;
	bool wait;
	Screen scr;
	Enemy E; // manager
	Hero hero;
	Barrage barr;	
	Bullet B;

	std::vector<Enemy*> enemies;
	std::queue<enemyData> waveQueue; // 使用队列管理波次
	enemyData currentWave;

	DWORD waitStart;

public:
	Game();
	~Game();

	void Touhou();
	void InitLevels();
	void HandleRound();

	void Bullets();	
	void HeroControl();	

	void Barrages(std::vector<Enemy*>& enemyList);
	void Enemies(std::vector<Enemy*>& enemyList);
};

