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
		if (!waveQueue.empty()) {
			wave = waveQueue.front();
			waveQueue.pop();
			E.InitRound();
			E.SetStatus(wave.position, wave.interval);
		}
	}
	Barrages();
	if (wait) {
		if (now - waitStart >= (DWORD)wave.waveDelay) wait = false;
		else return;
	}
	Enemies();
}

void Game::InitLevels() {
	// Initialize enemy configurations for each round
	/* 参数顺序:  
	*   敌人类型, 
	*	弹幕类型, 
	*	间隔  
	*	等待
	*	弹幕速度
	*	位置
	* 	param[0]: number of bullets in firework barrage
	*   param[1]: gap/time interval
	*   param[2]: omega for windmill
	*   param[3]: radius for circle mill
	*	param[4]: linear velocity for wheel	
	*/
	waveQueue.push({
		eType::elf,
		bType::windmill_st,
		0,
		2000,
		4.0f,
		{CentralX},
		{4, 100, 5}
	});
	waveQueue.push({ 
		eType::elf,    
		bType::firework, 
		0,     
		2000, 
		3.0f,
		{CentralX},
		{12, 200}
	});
	waveQueue.push({
		eType::normal,
		bType::down_st,
		500,
		2000,
		5.0f,
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
	DWORD now = GetTickCount();

	// 1. 遍历所有敌人
	for (auto* en : E.getList()) {
		if (!en->isAlive()) continue;

		// 2. 如果敌人还没任务（刚生成），根据 wave 配置初始化任务
		// 这是一个简单的 "配置 -> 任务" 映射器
		if (en->GetTasks().empty()) {

			// 示例：如果 wave 类型是 wheel，我们给它一个 "组合弹幕"
			if (wave.barrageType == bType::wheel) {
				// 任务1：螺旋弹幕 (间隔200ms, 速度2.0, 角速度0.05, 数量5)
				en->AddTask(BarrageTask((int)bType::wheel, 200, 2.0f, 0.05f, 5));

				// 任务2：同时每2秒发射一次直线狙击 (间隔2000ms, 速度5.0, 数量10)
				en->AddTask(BarrageTask((int)bType::windmill_st, 2000, 5.0f, 10, 10));
			}
			// 示例：普通敌人
			else if (wave.barrageType == bType::down_st) {
				en->AddTask(BarrageTask((int)bType::down_st, 1000, 5.0f, 0, 1));
			}
			// 默认兜底：直接把 wave 的参数转为一个任务
			else {
				en->AddTask(BarrageTask((int)wave.barrageType, wave.param[1], wave.barrSpeed, wave.param[2], (int)wave.param[0], (int)wave.param[3]));
			}
		}

		// 3. 执行任务
		for (auto& task : en->GetTasks()) {
			// 检查时间间隔
			if (now - task.lastTime >= task.interval) {

				int x = en->x + Enemy::getElfWidth() / 2 - Barrage::getDarkGreenWidth() / 2;
				int y = en->y + Enemy::getElfHeight() / 2 - Barrage::getDarkGreenHeight() / 2;

				// 根据任务类型分发
				switch ((bType)task.type) {
				case bType::down_st:
					Barr.Normal(*en, task.speed);
					break;
				case bType::windmill_st:
					// 注意：这里用 task.omega 来传参
					Barr.straightMill(*en, task.speed, (int)task.omega, task.num, x, y, 1);
					break;
				case bType::firework:
					Barr.fireWork(*en, task.speed, task.num, x, y);
					break;
				case bType::circle_mill:
					Barr.circleMill(*en, task.speed, task.r, task.num, x, y);
					break;
				case bType::wheel:
					Barr.wheel(*en, task.speed, task.omega, task.num, x, y);
					break;
				}

				// 更新时间
				task.lastTime = now;
			}
		}
	}

	Barr.update();
}

void Game::Enemies() {
	E.createEnemy(wave.type);
	E.drawAll();
	E.moveEnemy();
	E.collision(wave.type, &B);
}




	