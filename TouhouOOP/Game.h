#ifndef GAME_H
#define GAME_H
#include "Library.h"
#include "Hero.h"	
#include "Bullet.h"
#include "Screen.h"
#include "Enemy.h"
#include "Barrage.h"

class Game {

	int round;	
	int aliveBullet;
	int barrageType;
	int enemyType;
	bool heroFire;
	bool enemyFire;
	bool wait;
	Screen scr;
	Enemy e;
	Enemy enemy1[15];
	Hero hero;
	Barrage barr1;	
	std::vector<Barrage> barr1List;
	std::vector<Bullet> bulletList;
	
	/*te : Enemy rate
	 *tb : Bullet rate
	 *tba: Barrage rate	
	 */
	DWORD te1, te2, tb1, tb2, tba1, tba2;


public:
	Game();
	void Touhou();
	void HandleRound();
	void createBullet();	
	void creatBarrage(Enemy* enemy, int type);

	int getEnemyType(int e);
	int getBarrageType(int r);	
};
#endif
