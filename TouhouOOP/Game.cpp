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
		if (waveQueue.empty()) { 
			return;
		}
		nextWave = waveQueue.front();
		wait = true;
		waitStart = now;
	}
	
	if (wait) {
		if (now - waitStart >= (DWORD)nextWave.waveDelay) {
			wait = false;
			waveQueue.pop();
			E.InitRound();
			E.setWave(nextWave.events);
		}
		else return;
	}
	Barrages();
	Enemies();
}

void Game::InitLevels() {
	// --- 第一波 (Wave 1) ---
	{
		waveData w1;
		w1.waveDelay = 2000; // 第一波开始前等待 2 秒（或者是上一波结束后的间隔）

		// 事件1配置...
		SpawnEvent e1;
		e1.startTime = 500; // 波次开始后 500ms 生成
		e1.count = 5; 		// 生成 5 个敌人
		e1.interval = 400;  // 每 400ms 生成一个
		e1.type = eType::normal; // 普通敌人
		e1.startX = 100; e1.startY = -50; // 起始位置
		e1.moveLogic = Moves::SineWave(100, 50, 2.0f, 3.0f); // 设定移动逻辑
		e1.initTasks.push_back(BarrageTask((int)bType::down_st, 500, 5.0f, 0, 1)); // 携带一个简单的下落弹幕任务

		w1.events.push_back(e1); // 加入到 WaveData 的事件列表

		// 事件2配置...
		SpawnEvent e2;
		e2.startTime = 2000;
		e2.count = 3;
		e2.interval = 600;
		e2.type = eType::elf;
		e2.startX = 400; e2.startY = -50;
		e2.moveLogic = Moves::Linear(0, 2.0f);
		e2.initTasks.push_back(BarrageTask((int)bType::windmill_st, 80, 4.0f, 3, 1));
		w1.events.push_back(e2);

		// 推入队列
		waveQueue.push(w1);
	}
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

		/*
		// 2. 如果敌人还没任务（刚生成），根据 wave 配置初始化任务
		// 这是一个简单的 "配置 -> 任务" 映射器
		if (en->GetTasks().empty()) {
			switch (currentWave.barrageType) {
			// 添加一个组合弹幕
			case bType::combo_1:
				en->AddTask(BarrageTask(
					(int)bType::pincer_aim, // type
					2000,                   // interval
					5.0f,                   // speed
					0,                      // omega (unused)
					2,                      // num (pairNum)
					80,                     // r (spacing)
					1,                      // dir
					5,                      // burstCount (连发8次)
					100,                     // burstInterval (间隔80ms)
					0.6f                    // acc (速度递减)
				));
				en->AddTask(BarrageTask(
					(int)bType::random_rain, // 类型
					3000,                    // interval: 每轮间隔 3000ms
					8.0f,                    // speed: 初始速度 8.0 (很快)
					0,                       // omega: 无用
					0,                       // num: 无用
					0,                       // r: 无用
					0,                       // dir: 无用
					20,                      // burstCount: 一轮连发 20 发
					50,                      // burstInterval: 每发间隔 50ms (很密)
					0.1f                     // speedDec: 每发速度减 0.1 (稍微有点层次感)
				));
				en->AddTask(BarrageTask((int)bType::firework, 3000, 4.0f, 0, 12));
				break;
			// 示例：普通敌人
			case bType::down_st:
				en->AddTask(BarrageTask((int)bType::down_st, 1000, 5.0f, 0, 1));
				break;
			// 默认兜底：直接把 wave 的参数转为一个任务
			default:
				en->AddTask(BarrageTask((int)wave.barrageType, wave.param[1], wave.barrSpeed, wave.param[2], (int)wave.param[0], (int)wave.param[3]));
			}
		}
		*/

		int centerX = (int)en->x + Enemy::getElfWidth() / 2 - Barrage::getDarkGreenWidth() / 2;
		int centerY = (int)en->y + Enemy::getElfHeight() / 2 - Barrage::getDarkGreenHeight() / 2;
		
		// 3. 执行任务
		for (auto& task : en->GetTasks()) {

			bool shouldFire = false;

			// 情况1：新的一轮开始 (Start New Wave)
			// 当不在连发状态 (currentBurst == 0) 且 达到了大间隔时间
			if (task.currentBurst == 0) {
				if (now - task.lastTime >= task.interval) {
					shouldFire = true;
					task.lastTime = now; // 重置大计时器
				}
			}
			// 情况2：连发中 (In Burst)
			// 当处于连发状态 (0 < currentBurst < Total) 且 达到了小间隔时间
			else if (task.currentBurst > 0 && task.currentBurst < task.burstCount) {
				if (now - task.lastBurstTime >= task.burstInterval) {
					shouldFire = true;
				}
			}

			// 调试pincer_aim 的挂载点
			if (task.type == (int)bType::pincer_aim) {
				COLORREF oldColor = getfillcolor();
				setfillcolor(RED);
				int spacing = task.r;
				int pairNum = task.num;
				for (int i = 1; i <= pairNum; ++i) {
					int offset = i * spacing;
					fillcircle(centerX - offset, centerY, 4); // 左点
					fillcircle(centerX + offset, centerY, 4); // 右点
				}
				setfillcolor(oldColor);
			}

			// 检查时间间隔
			if (!shouldFire) continue;

			float currentSpeed = task.speed - (task.currentBurst * task.acc);
			if (currentSpeed < 0.5f) currentSpeed = 0.5f; // 最低速度保护
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
			case bType::pincer_aim:
				Barr.pincerAim(*en, Hero.x, Hero.y, currentSpeed, task.r, task.num, x, y);
				break;
			case bType::random_rain:
				Barr.randomRain(currentSpeed);
				break;
			default:
				break;
			}
			task.currentBurst++;          // 计数+1
			task.lastBurstTime = now;     // 更新小计时器
			// 检查连发是否结束
			if (task.currentBurst >= task.burstCount) {
				task.currentBurst = 0;    // 重置，等待下一个大 interval
			}
		}
	}

	Barr.update();
}

void Game::Enemies() {
	E.moveEnemy();
	E.drawAll();
	E.collision(&B);
}




	