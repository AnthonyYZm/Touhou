#pragma once
#include "Role.h"
#include "Hero.h"	
#include "Bullet.h"
#include "Screen.h"
#include "Enemy.h"
#include "Barrage.h"
#include "EnemyManager.h"
#include "Boss.h"

class Game {

	static int bulletLevel;
	bool enemyFire;
	bool wait;
	Screen Scr;
	EnemyManager E;
	Hero Hero;
	Barrage Barr;	
	Bullet B;

	std::queue<waveData> waveQueue;
	std::vector<SpawnEvent> currentWave;
	waveData nextWave;

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
	
	void createBoss();	
};

