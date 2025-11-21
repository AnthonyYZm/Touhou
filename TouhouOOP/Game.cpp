#include "Game.h"
	

Game::Game() {
	te1 = 0;
	te2 = 0;
	enemyFire = false;
	round = 0;
	wait = false;	
	bulletLevel = 1;
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

void Game::Barrages(Enemy* enemy, bType type) {
	switch (type) {
		case bType::down_st:{
			// Handle barrage type 1
			barr.Normal(enemy, &e);
			break;
		}
		case bType::windmill_st:{
			// Handle barrage type 2
			static float angle = 90.0f;
			barr.Straight(enemy, 50, 5.0f, angle);
			break;
		}
		default:
			break;
	}
}

void Game::Enemies(eType type) {
	switch (type) {
	case eType::normal: {
		e.createEnemy(eType::normal, enemy1);
		e.move(eType::normal, enemy1, 4.5f);
		e.collision(eType::normal, &B, enemy1);
		break;
	}
	case eType::elf: {
		e.createEnemy(eType::elf, &enemy2);
		e.move(eType::elf, &enemy2, 3.0f);
		e.collision(eType::elf ,&B, &enemy2);
		break;
	}
	default:
		break;
	}
}

void Game::HandleRound() {
	te2 = GetTickCount();

	if (e.checkEnemyClear() && !wait) {
		wait = true;
		te1 = te2;
		clearRoundEnemy();
	}

	if (wait) {
		if (te2 - te1 >= 2000) {
			e.InitRound();
			e.EnemyX();
			e.EnemyNum();
			wait = false;
			te1 = GetTickCount();
			round++;
		}
		else {
			return;
		}
	}

	switch (round) {
		case 1:
			Enemies(eType::elf);
			Barrages(&enemy2, bType::windmill_st);
			break;
		case 2:
			Enemies(eType::elf);
			Barrages(&enemy2, bType::windmill_st);
			break;
		case 3:
			Enemies(eType::normal);
			Barrages(enemy1, bType::down_st);
			break;
		default:
			Enemies(eType::normal);
			Barrages(enemy1, bType::down_st);
			break;
	}
}	

void Game::clearRoundEnemy() {
	// 2. 清理 Boss (currentBoss)
	if (Boss1 != nullptr) {
		delete Boss1; // 释放内存
		Boss1 = nullptr; // 将指针设为 nullptr
	}
}
	