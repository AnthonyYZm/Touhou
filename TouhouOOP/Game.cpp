#include "Game.h"
	

Game::Game() {
	te1 = 0;
	te2 = 0;
	tb1 = GetTickCount();
	tb2 = 0;
	heroFire = false;
	enemyFire = false;
	recordRound = 0;
	aliveBullet = 0;
	wait = false;	
}

void Game::Touhou() {
	scr.gameScreen();
	BeginBatchDraw();

	while (IsWindow(GetHWnd())) {
		cleardevice();
		//scr.gameBackground();
		//Game logic
		HandleRound();

		hero.draw();
		hero.move();
		
		createBullet();

		FlushBatchDraw();
		Sleep(1);
	}
	EndBatchDraw();
	closegraph();
}

void Game::createBullet() {
	if (GetAsyncKeyState('Z') & 0x8000) {
		heroFire = true;
		tb2 = GetTickCount();
		if (tb2 - tb1 >= 100) {
			Bullet Bullet(hero.x + 16, hero.y);
			Bullet.Alive = true;
			bulletList.push_back(Bullet);
			tb1 = tb2;
		}
	}
	heroFire = false;

	for (auto & b : bulletList) {
		if (b.Alive) {
			b.draw();
			b.move();
		}
	}
}


void Game::HandleRound() {
	te2 = GetTickCount();

	// 1️⃣ 检查是否清空且尚未等待
	if (e.checkEnemyClear(enemy1) && !wait) {
		wait = true;       
		te1 = te2;         
		recordRound++;
	}

	// 2️⃣ 等待 2 秒后再重置并生成新敌人
	if (wait && te2 - te1 >= 2000) {
		e.InitRound();  
		e.EnemyX();
		e.EnemyNum();
		e.Init();
		wait = false;
	}
	e.createEnemy(enemy1);
	e.move(2.8f, enemy1);
	e.collision(bulletList, enemy1);
}	