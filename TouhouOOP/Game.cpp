#include "Game.h"
	

Game::Game() {
	tb1 = GetTickCount();
	tb2 = 0;
	heroFire = false;
	enemyFire = false;
	round = 1;
	aliveBullet = 0;
	wait = false;	
	barrageType = 0;
}

void Game::Touhou() {
	scr.gameScreen();
	BeginBatchDraw();

	while (IsWindow(GetHWnd())) {
		cleardevice();
		//scr.gameBackground();
		//Game logic
		HandleRound();
		creatBarrage(enemy1, 1);
		hero.draw();
		hero.move();
		createBullet();

		FlushBatchDraw();
		Sleep(1);
	}
	EndBatchDraw();
	closegraph();
}

int Game::getBarrageType(int e) {
	switch (e) {
	case 4:
		return 1;
		break;
	default:
		return 1;
		break;
	}
}	

int Game::getEnemyType(int r) {
	switch (r) {
		case 4:
			return 1;
			break;
		default:
			return 1;	
			break;
	}
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


void Game::creatBarrage(Enemy* enemy, int type) {
	switch (getBarrageType(getEnemyType(round))) {
		case 1:
			// Handle barrage type 1
			tba2 = GetTickCount();
			if (e.GetAliveEnemy() == e.getEnemyNum() && tba2 - tba1 >= 1000) {
				int cnt = e.GetAliveEnemy();
				for (int i = 0; i < cnt; i++) {
					if (!enemy[i].isAlive()) continue;
					Barrage b(enemy[i].x, enemy[i].y + 20);
					b.alive = true;
					barr1List.push_back(b);
				}
				tba1 = tba2;
			}
			for (auto& b : barr1List) {
				if (b.isAlive()) {
					b.draw();
					b.move();
				}
			}
			break;
	}
}

void Game::HandleRound() {
	te2 = GetTickCount();

	// 1️⃣ 检查是否清空且尚未等待
	if (e.checkEnemyClear(enemy1) && !wait) {
		wait = true;       
		te1 = te2;         
		round++;
	}

	// 2️⃣ 等待 2 秒后再重置并生成新敌人
	if (wait && te2 - te1 >= 2000) {
		e.InitRound();  
		e.EnemyX();
		e.EnemyNum();
		e.Init();
		wait = false;
		tba1 = GetTickCount(); // 重置弹幕计时器
	}
	e.createEnemy(getEnemyType(round), enemy1);
	e.move(getEnemyType(round), enemy1);
	e.collision(bulletList, enemy1);
}	