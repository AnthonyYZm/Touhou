#include "Game.h"

int Game::bulletLevel = 1;

Game::Game() {
	enemyFire = false;
	wait = false;	
	bulletLevel = 1;
	InitLevels();
}

Game::~Game() {
	for (auto* e : enemies) {
		delete e;
	}
	enemies.clear();
}

void Game::Touhou() {
	scr.gameScreen();
	BeginBatchDraw();

	while (IsWindow(GetHWnd())) {
		cleardevice();
		//scr.gameBackground();
		
		//Game logic
		HandleRound();
		Bullets();
		HeroControl();
		FlushBatchDraw();
		Sleep(1);
	}

	EndBatchDraw();
	closegraph();
}

void Game::HandleRound() {
	DWORD now = GetTickCount();

	if (E.checkEnemyClear() && !wait) {
		wait = true;
		waitStart = now;

		// 确保 vector 是空的
		for (auto* ptr : enemies) {
			delete ptr;
		}
		enemies.clear();

		// 胜利检测，暂时空置
		if (waveQueue.empty()) {
		}

		// 取出下一波配置
		currentWave = waveQueue.front();
		waveQueue.pop();
		E.InitRound();
		E.EnemyX();
		E.EnemyNum();
		E.SetStatus(currentWave.enemyCount, currentWave.interval);
	}

	if (wait) {
		if (now - waitStart >= (DWORD)currentWave.waveDelay) wait = false;
		else return;
	}
	Enemies(enemies);
	Barrages(enemies);
}

void Game::InitLevels() {
	// Initialize enemy configurations for each round
	// 参数顺序:      敌人类型,          弹幕类型,     数量 出怪间隔 波次前间隔
	waveQueue.push({ eType::elf,    bType::windmill_st, 1,     0,      2000 }); 
	waveQueue.push({ eType::elf,    bType::windmill_st, 1,     0,      2000 }); 
	waveQueue.push({ eType::normal, bType::down_st,     5,   1000,     2000 }); 
	waveQueue.push({ eType::normal, bType::down_st,     5,   1000,     2000 }); 
}

void Game::HeroControl() {
	hero.draw();
	hero.move();
}

void Game::Bullets() {
	if (GetAsyncKeyState('Z') & 0x8000) {
		B.setFire(true);	
	}
	B.createBullet(&hero, bulletLevel);
}

void Game::Barrages(std::vector<Enemy*>& enemyList) {
	switch (currentWave.barrageType) {
		case bType::down_st:
			barr.Normal(enemyList, &E);
			break;
		case bType::windmill_st:{
			static float angle = 90.0f;
			for (auto* en : enemyList) {
				if(en->isAlive()) barr.Straight(en, 50, 5.0f, angle);
			}
			break;
		}
		default:
			break;
	}
}

void Game::Enemies(std::vector<Enemy*>& enemyList) {
	E.createEnemy(currentWave.type, enemyList);
	E.move(currentWave.type, enemyList, Enemy::speedMap[currentWave.type]);
	E.collision(currentWave.type, &B, enemyList);
}





	