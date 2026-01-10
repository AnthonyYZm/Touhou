#include "Game.h"

int Game::bulletLevel = 1;

Game::Game() {
	enemyFire = false;
	wait = false;	
	bulletLevel = 1;
	InitLevels();
}

Game::~Game() {
}

void Game::Touhou() {
	Scr.gameScreen();
	BeginBatchDraw();

	while (IsWindow(GetHWnd())) {
		cleardevice();
		//Scr.gameBackground();
		
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

		// 胜利检测，暂时空置
		if (waveQueue.empty()) {
		}

		// 取出下一波配置
		currentWave = waveQueue.front();
		waveQueue.pop();
		E.InitRound();
		E.SetStatus(currentWave.position, currentWave.interval);
	}

	if (wait) {
		if (now - waitStart >= (DWORD)currentWave.waveDelay) wait = false;
		else return;
	}
	Enemies();
	Barrages();
}

void Game::InitLevels() {
	// Initialize enemy configurations for each round
	/* 参数顺序:  
	*   敌人类型, 
	*	弹幕类型, 
	*	间隔  
	*	等待      
	*	敌人
	*/
	waveQueue.push({ 
		eType::elf,    
		bType::windmill_st, 
		0,     
		2000,     
		{CentralX} 
	});
	waveQueue.push({
		eType::elf,
		bType::windmill_st,
		0,
		2000,
		{CentralX}
	});
	waveQueue.push({
		eType::normal,
		bType::down_st,
		500,
		2000,
		{100.0f, 100.0f, 100.0f, 100.0f} 
	});
}

void Game::HeroControl() {
	Hero.draw();
	Hero.move();
}

void Game::Bullets() {
	if (GetAsyncKeyState('Z') & 0x8000) {
		B.setFire(true);	
	}
	B.createBullet(&Hero, bulletLevel);
}

void Game::Barrages() {
	switch (currentWave.barrageType) {
		case bType::down_st:
			Barr.Normal(E.getList(), &E);
			break;
		case bType::windmill_st:{
			static float angle = 90.0f;
			Barr.straightMill(E.getList(), 50, 5.0f, angle);
			break;
		}
		default:
			break;
	}
	Barr.update();
}

void Game::Enemies() {
	E.createEnemy(currentWave.type);
	E.drawAll(currentWave.type);
	E.move(currentWave.type, Enemy::speedMap[currentWave.type]);
	E.collision(currentWave.type, &B);
}




	