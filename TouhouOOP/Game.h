#pragma once
#include "Role.h"
#include "Hero.h"	
#include "Bullet.h"
#include "Screen.h"
#include "Enemy.h"
#include "Barrage.h"

class Game {

	unsigned int round;	
	//int barrageType;
	//int enemyType;
	unsigned int bulletLevel;	
	bool enemyFire;
	bool wait;
	Screen scr;
	Enemy e;
	Enemy enemy1[10];
	Enemy enemy2;
	Hero hero;
	Barrage barr;	
	Bullet B;
	std::vector<Enemy*> NormalEnemies;
	Enemy* Boss1 = nullptr;
	/*te : Enemy rate
	 *tb : Bullet rate
	 *tba: Barrage rate	
	 */
	DWORD te1, te2, tb1, tb2, tba1, tba2;


public:
	Game();

	void Touhou();
	void HandleRound();
	void Bullets();	
	void HeroControl();	
	void Barrages(Enemy* enemy, bType type);
	void Enemies(eType type);
	void clearRoundEnemy();	
};

