#ifndef GAME_H
#define GAME_H
#include "Library.h"
#include "Hero.h"	
#include "Bullet.h"
#include "Screen.h"
#include "Enemy.h"

class Game {

	int recordRound;	
	int aliveBullet;
	bool heroFire;
	bool enemyFire;
	bool wait;
	Screen scr;
	Enemy e;
	Enemy enemy1[15];
	Hero hero;
	std::vector<Bullet> bulletList;
	
	/*te : Enemy rate
	 *tb : Bullet rate
	 */
	DWORD te1, te2, tb1, tb2;


public:
	Game();

	void Touhou();

	void HandleRound();

	void createBullet();	

};
#endif
