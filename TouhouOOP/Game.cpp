#include "Game.h"
#include <chrono>
#include <thread>
#include <mmsystem.h>
#include <iostream>
#pragma comment(lib, "winmm.lib")


int Game::bulletLevel = 1;
AudioManager Game::Audio;
EffectManager Game::Effects;
BackgroundManager Game::BG;

Game::Game() {

	SetConsoleOutputCP(CP_UTF8);

	enemyFire = false;
	wait = false;
	bulletLevel = 1;
	InitNormalLevels();    
	InitAILevel();
	isSpellActive = false;
	spellAngle = 0.0f;
	spellRadius = 10.0f;
	Audio.init();
	Effects.init();
	BG.init();

	// 初始化状态为主菜单
	currentState = GameState::MAIN_MENU;

	// 初始化 AI 控制器为空指针
	aiController = nullptr;
}


Game::~Game() {
	ClearSpellBarrages();

	// 清理 AI 控制器
	if (aiController) {
		delete aiController;
		aiController = nullptr;
	}

	if (aiProcessHandle != NULL) {
		TerminateProcess(aiProcessHandle, 0);
		CloseHandle(aiProcessHandle);
		aiProcessHandle = NULL;
	}
}

void Game::Touhou() {
	initgraph(screenWidth, screenHeight);
	adjustWindow();
	setbkcolor(WHITE);
	BeginBatchDraw();

	// 创建离屏缓冲区（固定游戏分辨率）
	HDC easxDC = GetImageHDC();
	offDC  = CreateCompatibleDC(easxDC);
	offBmp = CreateCompatibleBitmap(easxDC, screenWidth, screenHeight);
	offOld = (HBITMAP)SelectObject(offDC, offBmp);

	using clock = std::chrono::high_resolution_clock;
	constexpr auto TARGET_FRAME_TIME = std::chrono::microseconds(1000000 / 60);
	constexpr auto SLEEP_THRESHOLD = std::chrono::microseconds(2000);

	timeBeginPeriod(1);
	while (IsWindow(GetHWnd())) {
		const auto frameStart = clock::now();
		cleardevice();

		// 根据当前状态执行不同逻辑
		switch (currentState) {
		case GameState::MAIN_MENU:
			HandleMenuInput();
			DrawMainMenu();
			break;

		case GameState::NORMAL_PLAY:
			BG.update();
			BG.draw();
			HandleRound();
			Effects.update();
			Effects.draw();
			HeroControl();
			Bullets();
			drawUI();
			break;

		case GameState::AI_DEMO:
			HandleAIDemo();
			break;
		}

		// 先把 EasyX 批绘制内容刷到 EasyX 内部 DC，再缩放输出到窗口
		FlushBatchDraw();

		while (true) {
			const auto now = clock::now();
			const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - frameStart);
			const auto remain = TARGET_FRAME_TIME - elapsed;
			if (remain <= std::chrono::microseconds(0)) break;
			if (remain > SLEEP_THRESHOLD) {
				Sleep(1);
			}
			else {
				break;
			}
		}

		while (std::chrono::duration_cast<std::chrono::microseconds>(clock::now() - frameStart) < TARGET_FRAME_TIME) {
			std::this_thread::yield();
		}
	}
	timeEndPeriod(1);

	// 释放离屏缓冲区
	SelectObject(offDC, offOld);
	DeleteObject(offBmp);
	DeleteDC(offDC);
	offDC = nullptr; offBmp = nullptr; offOld = nullptr;

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
	}
	Barrages(); 
	Enemies();  
	UpdateItems(); 
	UpdateSpellCard();
	CheckCollision();
	updateBoss();
	handleBGM();
}

void Game::handleBGM() {
	bool isBossAlive = false;
	for (auto* e : E.getList()) {
		if (e == nullptr) continue;
		if (e->type == eType::boss && e->isAlive()) {
			isBossAlive = true;
			break;
		}
	}
	if (isBossAlive) {
		Game::Audio.playBGM(L"bgm_sanae");
	}
	else {
		Game::Audio.playBGM(L"bgm_stage1");

	}
}

void Game::InitNormalLevels() {

	{
		// 第一波
		waveData w1;
		w1.waveDelay = 2000; // 第一波开始前等待 2 秒
		SpawnEvent e1;
		e1.startTime = 500; // 波次开始后 500ms 生成
		e1.count = 10; 		// 生成 5 个敌人
		e1.interval = 400;  // 每 400ms 生成一个
		e1.hp = 1;
		e1.type = eType::normal; // 普通敌人
		e1.startX = CentralX; e1.startY = TopEdge; // 起始位置
		e1.moveLogic = Moves::SineWave(CentralX, 50, 2.0f, 3.0f); // 设定移动逻辑
		//e1.initTasks.push_back(BarrageTask((int)bType::down_st, 500, 5.0f, 0, 1)); // 弹幕任务
		w1.events.push_back(e1);
		waveQueue.push(w1);

		// 第三波
		waveData w3;
		w3.waveDelay = 2000;

		SpawnEvent e4;
		e4.startTime = 2000;
		e4.count = 50;
		e4.type = eType::normal;
		e4.hp = 1;
		e4.interval = 100;
		e4.startX = LeftEdge; e4.startY = CentralY - 50;
		e4.moveLogic = Moves::shuttle(10, 1);
		//e4.initTasks.push_back(BarrageTask((int)bType::down_st, 300, 2.0f, 0, 1));

		SpawnEvent e5;
		e5.startTime = 2000;
		e5.count = 50;
		e5.type = eType::normal;
		e5.hp = 1;
		e5.interval = 100;
		e5.startX = LeftEdge + WIDTH; e5.startY = CentralY + 50;
		e5.moveLogic = Moves::shuttle(10, 0);
		e5.initTasks.push_back(BarrageTask((int)bType::down_st, 800, 2.0f, 0, 1));
		w3.events.push_back(e4);
		w3.events.push_back(e5);
		waveQueue.push(w3);

		// 第四波
		waveData w4;
		w4.waveDelay = 2000;
		SpawnEvent e6;
		e6.startTime = 500;
		e6.count = 20;
		e6.interval = 400;
		e6.type = eType::normal;
		e6.hp = 1;
		e6.startX = CentralX - 200; e6.startY = TopEdge;
		e6.moveLogic = Moves::SineWave(CentralX - 200, 50, 2.0f, 3.0f);

		SpawnEvent e7;
		e7.startTime = 500;
		e7.count = 20;
		e7.interval = 400;
		e7.type = eType::normal;
		e7.hp = 1;
		e7.startX = CentralX + 200; e7.startY = TopEdge;
		e7.moveLogic = Moves::SineWave(CentralX + 200, 50, 2.0f, 3.0f);

		SpawnEvent e8;
		e8.startTime = 5500;
		e8.count = 1;
		e8.type = eType::elf;
		e8.hp = 20;
		e8.startX = CentralX; e8.startY = TopEdge;
		e8.moveLogic = Moves::Hover(CentralY, 2.0f);
		e8.initTasks.push_back(BarrageTask((int)bType::firework, 500, 3.0f, 0, 8));

		SpawnEvent e9;
		e9.startTime = 10500;
		e9.count = 1;
		e9.type = eType::elf;
		e9.hp = 10;
		e9.startX = CentralX - 100; e9.startY = TopEdge;
		e9.moveLogic = Moves::Hover(CentralY, 2.0f);
		e9.initTasks.push_back(BarrageTask((int)bType::firework, 500, 3.0f, 0, 8));

		SpawnEvent e10;
		e10.startTime = 3000;
		e10.count = 1;
		e10.type = eType::elf;
		e10.hp = 20;
		e10.startX = CentralX + 100; e10.startY = TopEdge;
		e10.moveLogic = Moves::Hover(CentralY, 2.0f);
		e10.initTasks.push_back(BarrageTask((int)bType::firework, 500, 3.0f, 0, 8));

		w4.events.push_back(e6);
		w4.events.push_back(e7);
		w4.events.push_back(e8);
		w4.events.push_back(e9);
		w4.events.push_back(e10);
		waveQueue.push(w4);
	}

	
	{
		waveData wBoss;
		wBoss.waveDelay = 2000;

		SpawnEvent boss;
		boss.startTime = 500;
		boss.count = 1;
		boss.interval = 0;
		boss.type = eType::boss;

		Boss* sanae = new Boss(LeftEdge, TopEdge);
		sanae->type = eType::boss;

		sanae->onEvent = [&](float x, float y, int id) {
			if (id == 1) { // 符卡展开
				Game::Audio.play(L"spell");
				Game::Effects.spawn(EffectType::SPELL_CUTIN, 0, 0, false);
				Game::Effects.spawn(EffectType::SPELL_NAME, 0, 0, false);
				Game::BG.setMode(BGMode::BOSS_SPELL);
				Game::Barr.clearBarrage();
			}
			else if (id == 2) { // 阶段击破
				Game::Audio.play(L"damage");
				Game::Effects.spawn(EffectType::EXPLOSION, x, y);
				Game::BG.setMode(BGMode::NORMAL);
				EnemyManager::DropReq req = { x, y, 10 };
				Game::E.dropQueue.push_back(req);
				Game::Effects.clearSpellName();
				Game::Barr.clearBarrage();
			}
			};
		// phase1
		{
			BossPhase p1(
				600,   // 血量
				30000,  // 限时 30秒
				Moves::bossEnter(80.0f), // 移动逻辑：飞到 Y=100 处悬停
				false,
				1,
				2000
			);
			// phase1
			//omega = 1.618 * 
			p1.tasks.push_back(BarrageTask(
				(int)bType::star_fall,
				5000,     // interval 发射间隔 (越小线条越密集)                     
				0.3f,      // speed 拉伸速度，控制线段变长的快慢                
				162.0f,   // omega 大圆半径，5个星星离Boss多远                  
				5,		  // num 5个星星                 
				1500,     // r 悬停时间 1500ms                   
				1,        // dir (未使用)                 
				100,      // x0 星星半径，控制五角星大小                   
				0,        // y0 (未使用)                  
				0.02f,     // acc 抛射加速度，控制星星炸开的力度，0.1~0.2 之间效果最好                   
				40,       // burstCount 总共画100笔，越多越细腻                 
				15,       // burstInterval 每笔间隔10ms，作画总耗时 1秒
				(int)BulletStyle::BLUE_
			));
			sanae->addPhase(p1);
		}

		// phase2
		{
			BossPhase p2(
				600,
				45000,
				Moves::Stay(),
				true,
				1000
			);
			int left = CentralX - 200;
			int right = CentralX + 200;
			p2.tasks.push_back(BarrageTask((int)bType::windmill_st, 40, 3.0f, 20.3f, 1, 0, 1, left,
				0, 0, 0, 0, (int)BulletStyle::RICE_BULE));
			p2.tasks.push_back(BarrageTask((int)bType::windmill_st, 40, 3.0f, 20.3f, 1, 0, 0, right,
				0, 0, 0, 0, (int)BulletStyle::RICE_BULE));
			p2.tasks.push_back(BarrageTask((int)bType::pincer_aim, 800, 4.0f, 0, 1,
				150, 1, 0, 0, 0, 1, 0, (int)BulletStyle::RED_));
			sanae->addPhase(p2);
		}

		// phase3
		BossPhase p3(
			600,
			45000,
			Moves::StepLeftUp(3000, 200, 5, 5),
			false,
			0
		);
		p3.tasks.push_back(BarrageTask(
			(int)bType::windmill_switching,
			100,             // interval 发射间隔 (越小线条越密集)
			3.0f,           // speed 子弹飞行速度
			8.0f,           // omega 旋转角速度 (每发旋转4度)
			8,              // num 几条旋臂 
			200,              // 停顿
			1,              // dir 初始方向 (1:顺时针)
			0, 0,           // x0, y0
			0,              // burstCount (未使用)
			0,        // burstCount (unused)
			1000,      // burstInterval (switch time, 3000ms)
			(int)BulletStyle::RICE_BULE
		));
		p3.tasks.push_back(BarrageTask(
			(int)bType::windmill_switching,
			100,             // interval 发射间隔 
			3.0f,           // speed 子弹飞行速度
			8.0f,           // omega 旋转角速度 
			8,              // num 几条旋臂 
			200,              // 停顿
			-1,              // dir 初始方向 
			0, 0,           // x0, y0
			0,              // burstCount (未使用)
			0,        // burstCount (unused)
			1000,      // burstInterval 
			(int)BulletStyle::RICE_RED
		));
		sanae->addPhase(p3);
		// phase4
		{
			BossPhase p4(
				600,
				45000,
				Moves::MoveTo(CentralX, CentralY, 80.0f),
				true
			);

			p4.tasks.push_back(BarrageTask((int)bType::firework, 500, 3.0f, 0, 24, 0, 0, 0, 0, 0, 0, 0, (int)BulletStyle::RICE_BULE));
			p4.tasks.push_back(BarrageTask((int)bType::pincer_aim, 1500, 4.0f, 0, 2,
				150, 1, 0, 0, 0.5f, 5, 80));
			sanae->addPhase(p4);
		}

		// phase5
		{
			BossPhase p5(
				600,
				60000,
				Moves::Stay(),
				false,
				1000
			);
			p5.tasks.push_back(BarrageTask(
				(int)bType::star_fall,
				5000,
				0.5,
				370.0f,
				12,
				1500,
				1,
				100,
				0,
				0.02f,
				40,
				15,
				(int)BulletStyle::RED_
			));
			p5.tasks.push_back(BarrageTask(
				(int)bType::star_fall,
				5000,     // interval 发射间隔 (越小线条越密集)                     
				0.3f,      // speed 拉伸速度，控制线段变长的快慢                
				162.0f,   // omega 大圆半径，5个星星离Boss多远                  
				5,		  // num 5个星星                 
				1500,     // r 悬停时间 1500ms                   
				1,        // dir (未使用)                 
				100,      // x0 星星半径，控制五角星大小                   
				0,        // y0 (未使用)                  
				0.02f,     // acc 抛射加速度，控制星星炸开的力度，0.1~0.2 之间效果最好                   
				40,       // burstCount 总共画100笔，越多越细腻                 
				15,       // burstInterval 每笔间隔10ms，作画总耗时 1秒
				(int)BulletStyle::BLUE_
			));
			sanae->addPhase(p5);
		}

		// phase6
		{
			BossPhase p6(
				800,
				45000,
				Moves::MoveTo(CentralX, CentralY, 80.0f),
				true,
				1000
			);
			//p6.tasks.push_back(BarrageTask((int)bType::firework, 800, 1.5f, 0, 15, 0, 0, 0, 0, 0, 0, 0, (int)BulletStyle::BLUE_BIG));
			p6.tasks.push_back(BarrageTask((int)bType::windmill, 350, 1.5f, 8, 6, 0, 0, 0, 0, 0, 1, 0, (int)BulletStyle::BLUE_BIG));
			p6.tasks.push_back(BarrageTask((int)bType::windmill_st, 100, 3.0f, 15, 12));
			sanae->addPhase(p6);
		}
		boss.bossInstance = sanae;
		wBoss.events.push_back(boss);
		waveQueue.push(wBoss);
	}
}

void Game::InitAILevel() {
	{
		waveData wBoss;
		wBoss.waveDelay = 2000;

		SpawnEvent boss;
		boss.startTime = 500;
		boss.count = 1;
		boss.interval = 0;
		boss.type = eType::boss;

		Boss* sanae = new Boss(LeftEdge, TopEdge);
		sanae->type = eType::boss;

		sanae->onEvent = [&](float x, float y, int id) {
			if (id == 1) { // 符卡展开
				Game::Audio.play(L"spell");
				Game::Effects.spawn(EffectType::SPELL_CUTIN, 0, 0, false);
				Game::Effects.spawn(EffectType::SPELL_NAME, 0, 0, false);
				Game::BG.setMode(BGMode::BOSS_SPELL);
				Game::Barr.clearBarrage();
			}
			else if (id == 2) { // 阶段击破
				Game::Audio.play(L"damage");
				Game::Effects.spawn(EffectType::EXPLOSION, x, y);
				Game::BG.setMode(BGMode::NORMAL);
				EnemyManager::DropReq req = { x, y, 10 };
				Game::E.dropQueue.push_back(req);
				Game::Effects.clearSpellName();
				Game::Barr.clearBarrage();
			}
			};
		// phase1
		{
			BossPhase p1(
				600,     // 血量
				30000,   // 限时 30秒
				Moves::bossEnter(80.0f), // 移动逻辑：飞到 Y=100 处悬停
				false,
				1,
				2000
			);
			// phase1
			//omega = 1.618 * 
			p1.tasks.push_back(BarrageTask(
				(int)bType::star_fall,
				5000,     // interval 发射间隔 (越小线条越密集)                     
				0.3f,     // speed 拉伸速度，控制线段变长的快慢                
				162.0f,   // omega 大圆半径，5个星星离Boss多远                  
				5,		  // num 5个星星                 
				1500,     // r 悬停时间 1500ms                   
				1,        // dir (未使用)                 
				100,      // x0 星星半径，控制五角星大小                   
				0,        // y0 (未使用)                  
				0.02f,    // acc 抛射加速度，控制星星炸开的力度，0.1~0.2 之间效果最好                   
				40,       // burstCount 总共画100笔，越多越细腻                 
				15,       // burstInterval 每笔间隔10ms，作画总耗时 1秒
				(int)BulletStyle::BLUE_
			));
			sanae->addPhase(p1);
		}

		// phase2
		{
			BossPhase p2(
				600,
				45000,
				Moves::Stay(),
				true,
				1000
			);
			int left = CentralX - 200;
			int right = CentralX + 200;
			p2.tasks.push_back(BarrageTask((int)bType::windmill_st, 40, 3.0f, 20.3f, 1, 0, 1, left,
				0, 0, 0, 0, (int)BulletStyle::RICE_BULE));
			p2.tasks.push_back(BarrageTask((int)bType::windmill_st, 40, 3.0f, 20.3f, 1, 0, 0, right,
				0, 0, 0, 0, (int)BulletStyle::RICE_BULE));
			p2.tasks.push_back(BarrageTask((int)bType::pincer_aim, 800, 4.0f, 0, 1,
				150, 1, 0, 0, 0, 1, 0, (int)BulletStyle::RED_));
			sanae->addPhase(p2);
		}

		// phase3
		BossPhase p3(
			600,
			45000,
			Moves::StepLeftUp(3000, 200, 5, 5),
			false,
			0
		);
		p3.tasks.push_back(BarrageTask(
			(int)bType::windmill_switching,
			100,             // interval 发射间隔 (越小线条越密集)
			3.0f,           // speed 子弹飞行速度
			8.0f,           // omega 旋转角速度 (每发旋转4度)
			8,              // num 几条旋臂 
			200,              // 停顿
			1,              // dir 初始方向 (1:顺时针)
			0, 0,           // x0, y0
			0,              // burstCount (未使用)
			0,        // burstCount (unused)
			1000,      // burstInterval (switch time, 3000ms)
			(int)BulletStyle::RICE_BULE
		));
		p3.tasks.push_back(BarrageTask(
			(int)bType::windmill_switching,
			100,             // interval 发射间隔 
			3.0f,           // speed 子弹飞行速度
			8.0f,           // omega 旋转角速度 
			8,              // num 几条旋臂 
			200,              // 停顿
			-1,              // dir 初始方向 
			0, 0,           // x0, y0
			0,              // burstCount (未使用)
			0,              // burstCount (unused)
			1000,      // burstInterval 
			(int)BulletStyle::RICE_RED
		));
		sanae->addPhase(p3);
		// phase4
		{
			BossPhase p4(
				600,
				45000,
				Moves::MoveTo(CentralX, CentralY, 80.0f),
				true
			);

			p4.tasks.push_back(BarrageTask((int)bType::firework, 500, 3.0f, 0, 24, 0, 0, 0, 0, 0, 0, 0, (int)BulletStyle::RICE_BULE));
			p4.tasks.push_back(BarrageTask((int)bType::pincer_aim, 800, 4.0f, 0, 2,
				150, 1, 0, 0, 0.5f, 1, 80));
			sanae->addPhase(p4);
		}

		// phase5
		{
			BossPhase p5(
				600,
				60000,
				Moves::Stay(),
				false,
				1000
			);
			p5.tasks.push_back(BarrageTask(
				(int)bType::star_fall,
				5000,
				0.5,
				370.0f,
				12,
				1500,
				1,
				100,
				0,
				0.02f,
				40,
				15,
				(int)BulletStyle::RED_
			));
			p5.tasks.push_back(BarrageTask(
				(int)bType::star_fall,
				5000,     // interval 发射间隔 (越小线条越密集)                     
				0.3f,     // speed 拉伸速度，控制线段变长的快慢                
				162.0f,   // omega 大圆半径，5个星星离Boss多远                  
				5,		  // num 5个星星                 
				3000,     // r 悬停时间 3000ms                   
				1,        // dir (未使用)                 
				100,      // x0 星星半径，控制五角星大小                   
				0,        // y0 (未使用)                  
				0.02f,    // acc 抛射加速度，控制星星炸开的力度，0.1~0.2 之间效果最好                   
				40,       // burstCount 总共画100笔，越多越细腻                 
				15,       // burstInterval 每笔间隔10ms，作画总耗时 1秒
				(int)BulletStyle::BLUE_
			));
			sanae->addPhase(p5);
		}

		// phase6
		{
			BossPhase p6(
				800,
				45000,
				Moves::MoveTo(CentralX, CentralY, 80.0f),
				true,
				1000
			);
			//p6.tasks.push_back(BarrageTask((int)bType::firework, 800, 1.5f, 0, 15, 0, 0, 0, 0, 0, 0, 0, (int)BulletStyle::BLUE_BIG));
			p6.tasks.push_back(BarrageTask((int)bType::windmill, 350, 1.5f, 8, 6, 0, 0, 0, 0, 0, 1, 0, (int)BulletStyle::BLUE_BIG));
			p6.tasks.push_back(BarrageTask((int)bType::windmill_st, 100, 3.0f, 15, 12));
			sanae->addPhase(p6);
		}
		boss.bossInstance = sanae;
		wBoss.events.push_back(boss);
		waveQueue.push(wBoss);
	}
}

void Game::HeroControl() {
	Hero.draw();
	Hero.move();

	static bool xPressed = false;
	if (GetAsyncKeyState('X') & 0x8000) {
		if (!xPressed) {
			if (Hero.tryUseBomb()) {
				CastSpellCard();
			}
			xPressed = true;
		}
	}
	else {
		xPressed = false;
	}
}

void Game::Bullets() {

	if (GetAsyncKeyState('Z') & 0x8000) {
		B.setFire(true);	
	}

	// AI 开火
	if (Hero.fire) {
		B.setFire(true);
	}
	B.createBullet(&Hero, bulletLevel, E.getList());
}

void Game::Barrages() {
	DWORD now = GetTickCount();

	for (auto* en : E.getList()) {
		if (en == nullptr) continue;
		if (!en->isAlive()) continue;
		if (!en->isFire()) continue;

		int centerX = (int)en->x;
		int centerY = (int)en->y;

		// 执行任务
		for (auto& task : en->GetTasks()) {

			BulletStyle style = static_cast<BulletStyle>(task.style);

			if (task.type == (int)bType::windmill_switching) {
				if (task.lastBurstTime == 0) {
					task.lastBurstTime = now;
					task.lastTime = now - task.interval;
				}
				if (task.currentBurst == 0) {
					// 这里的 interval 决定了发射密度
					if (now - task.lastTime >= (DWORD)task.interval) {
						task.lastTime = now; 
						task.currentAngle += task.omega * task.dir;
						Barr.directionalMill(*en, task.speed, task.currentAngle, task.num, centerX, centerY, task.dir, style);
					}

					if (now - task.lastBurstTime >= (DWORD)task.burstInterval) {
						task.currentBurst = 1;  
						task.lastBurstTime = now;
					}
				}
				else if (task.currentBurst == 1) {
					if (now - task.lastBurstTime >= task.r) {
						task.currentBurst = 0;    
						task.lastBurstTime = now;
						task.dir *= -1;          
					}
				}
				continue;
			}

			bool shouldFire = false;

			if (task.currentBurst == 0) {
				if (now - task.lastTime >= (DWORD)task.interval) {
					shouldFire = true;
					task.lastTime = now;
				}
			}
			// 连发
			else if (task.currentBurst > 0 && task.currentBurst < task.burstCount) {
				if (now - task.lastBurstTime >= (DWORD)task.burstInterval) {
					shouldFire = true;
				}
			}

			if (!shouldFire) continue;

			float currentSpeed = task.speed - (task.currentBurst * task.acc);
			if (currentSpeed < 0.5f) currentSpeed = 0.5f;

			int x = (task.x0 == 0) ? centerX : task.x0;
			int y = (task.y0 == 0) ? centerY : task.y0;

			switch ((bType)task.type) {
			case bType::down_st: 
				Barr.Normal(*en, task.speed, style);
				break;
			case bType::windmill_st: {
				float extraShift = 1.2f; // 每一发都额外多转 1.2 度
				task.currentAngle += (task.omega + extraShift);
				Barr.straightMill(*en, task.speed, (int)task.currentAngle, task.num, x, y, 1, style);
				break;
			}
			case bType::firework: 
				Barr.fireWork(*en, currentSpeed, task.num, x, y, style);
				break;
			case bType::circle_mill: 
				Barr.circleMill(*en, task.speed, task.r, task.num, x, y, style);
				break;
			case bType::windmill:
				Barr.straightMill2(*en, task.speed, (int)task.omega, task.num, x, y, 1, style);
				break;
			case bType::pincer_aim:
				Barr.pincerAim(*en, Hero.x, Hero.y, currentSpeed, task.r, task.num, x, y, style);
				break;
			case bType::random_rain: 
				Barr.randomRain(currentSpeed, style);
				break;
			case bType::star_fall: {
				DWORD releaseTime = task.lastTime + task.r;
				if (releaseTime < now) releaseTime = now;
				Barr.fiveStar(*en, task.currentBurst, task.burstCount, task.omega,
					(float)task.x0, task.speed, task.acc, releaseTime, task.num, centerX, centerY, style);
				break;
			}
			default: break;
			}

			task.currentBurst++;
			task.lastBurstTime = now;
			if (task.currentBurst >= task.burstCount) {
				task.currentBurst = 0;
			}
		}
	}
	Barr.update();
}

void Game::Enemies() {
	auto& enemies = E.getList();
	for (auto it = enemies.begin(); it != enemies.end(); ) {
		Enemy* e = *it;
		if (e == nullptr || !e->isAlive()) {
			if (e != nullptr) {
				delete e; 
			}
			it = enemies.erase(it); 
		}
		else {
			++it;
		}
	}
	E.moveEnemy();
	E.drawAll();
	E.outBound();
}

void Game::UpdateItems() {
	// 生成新道具 (从 EnemyManager 获取掉落请求)
	auto drops = E.popDrops();
	for (auto& d : drops) {
		for (int i = 0; i < d.count; ++i) {
			// 随机撒开一点
			float offX = (rand() % 20) - 10.0f;
			float offY = (rand() % 20) - 10.0f;
			Item* it = new Item(d.x + offX, d.y + offY);
			items.push_back(it);
		}
	}

	// 移动与拾取
	float heroCx = Hero.x + Hero::getWidth() / 2;
	float heroCy = Hero.y + Hero::getHeight() / 2;

	for (auto it = items.begin(); it != items.end(); ) {
		Item* item = *it;

		// 磁力判断
		float dx = item->x - heroCx;
		float dy = item->y - heroCy;
		if (dx * dx + dy * dy < 100 * 100) item->setMagnet(true);

		// 磁力移动逻辑
		if (item->isMagnet()) {
			// 简单的飞向自机算法
			float dx = heroCx - item->x;
			float dy = heroCy - item->y;
			float angle = atan2(dy, dx);
			float speed = 8.0f; // 吸附速度
			item->x += speed * cos(angle);
			item->y += speed * sin(angle);
		}
		else {
			item->move(); // 普通移动
		}

		item->draw();

		// 拾取检测 
		if (checkCircleCollide(item->x + 8, item->y + 8, 8, heroCx, heroCy, 16)) { 
			Hero.addPower(1);
			delete item;
			it = items.erase(it);
			continue;
		}
		// 越界删除
		if (item->y > HEIGHT) {
			delete item;
			it = items.erase(it);
		}
		else {
			++it;
		}
	}
}

void Game::CheckCollision() {

	float heroR = (float)Hero.JudgeR - 1;
	float bulletR = 6.0f;
	float barrageR = 5.0f;
	auto& enemies = E.getList();
	auto& bullets = B.bulletList;

	// 子弹 vs 敌人 
	for (auto& b : bullets) {
		if (!b->isAlive()) continue;

		float bCx = b->x;
		float bCy = b->y;

		for (auto it = enemies.begin(); it != enemies.end(); ) {
			Enemy* enemy = *it;
			if (!enemy->isAlive()) { ++it; continue; }

			float eCx = enemy->x;
			float eCy = enemy->y;
			float enemyR = 15.0f;
			if (enemy->type == eType::elf) enemyR = 32.0f;
			else if (enemy->type == eType::boss) enemyR = 30.0f; // 大精灵18，小兵12
			// 粗筛 (性能优化) 
			// 如果 x 或 y 轴的投影距离超过半径之和，绝对不可能相撞
			float rSum = bulletR + enemyR;
			float dx = std::abs(bCx - eCx);
			float dy = std::abs(bCy - eCy);

			if (dx > rSum || dy > rSum) {
				++it;
				continue;
			}
			if (checkCircleCollide(bCx, bCy, bulletR, eCx, eCy, enemyR)) {
				enemy->hp--;
				b->alive = false; 
				// 死亡逻辑
				if (enemy->hp <= 0) {
					// Boss 
					if (enemy->type == eType::boss) {
						// 死亡
						if (!enemy->isAlive()) {
							Game::BG.setMode(BGMode::NORMAL);
							Game::Effects.clearSpellName(); 
							Game::Audio.play(L"break");
							Game::Effects.spawn(EffectType::EXPLOSION, enemy->x, enemy->y);
							EnemyManager::DropReq req = { eCx, eCy, 20 };
							E.dropQueue.push_back(req);
							Hero::addScore(10000);
							delete enemy;
							it = enemies.erase(it);
							goto BulletHitBreak;
						}
						// 阶段结束
						goto BulletHitBreak;
					}

					// 如果是普通敌人
					else {
						Game::Audio.play(L"break");
						Game::Effects.spawn(EffectType::EXPLOSION, enemy->x, enemy->y);

						EnemyManager::DropReq req = { eCx, eCy, 1 };
						if (enemy->type == eType::elf) req.count = 5;
						E.dropQueue.push_back(req);
						Hero::addScore(1000); 

						delete enemy;
						it = enemies.erase(it);
						goto BulletHitBreak;
					}
				}
				// 没死
				goto BulletHitBreak;
			}
			++it;
		}
		continue; 
	BulletHitBreak:
		continue; 
	}

	// 弹幕 vs 自机 
	if (!Hero.isInvincible()) {
		for (auto* b : Barr.barrList) {
			if (!b->isAlive()) continue;
			// 弹幕判定半径 
			float barrCx = b->x;
			float barrCy = b->y;
			float bR = (float)b->getWidth() / 2; 
			float barrR = 0;
			if (b->getStyle() == BulletStyle::RICE_BULE || b->getStyle() == BulletStyle::RICE_RED) barrR = bR;
			else barrR = bR - 4;

			if (checkCircleCollide(Hero.x, Hero.y, heroR, barrCx, barrCy, barrR)) {
				Hero.hit();
				Game::Audio.play(L"dead");
				b->alive = false;
				break; 
			}
		}
	}

	// 敌人 vs 自机 
	if (!Hero.isInvincible()) {
		float heroR = (float)Hero.JudgeR;

		for (auto* e : E.getList()) {
			if (e == nullptr) continue;
			if (!e->isAlive()) continue;

			float eCx = e->x;
			float eCy = e->y;
			float eR = (float)std::min(e->width, e->height) / 3; 

			if (checkCircleCollide(Hero.x, Hero.y, heroR, eCx, eCy, eR)) {
				Hero.hit();
				Game::Audio.play(L"dead");
				break;
			}
		}
	}

	// 符卡碰撞逻辑
	if (isSpellActive && !spellBarrages.empty()) {

		// 符卡弹幕 vs 敌方弹幕
		for (auto& eb : Barr.barrList) { 
			if (!eb->alive) continue;
			float sbx = eb->x - Hero.x;
			float sby = eb->y - Hero.y;
			float dist = sbx * sbx + sby * sby;

			if (dist - (spellRadius + 100) * (spellRadius + 100) < 100) {
				if (eb->alive) {
					eb->alive = false;
					Game::Effects.spawn(EffectType::CLEAR_SMALL, eb->x, eb->y);
				}
			}
		}
		
		// 符卡弹幕 vs 敌人
		for (auto* enemy : E.getList()) {
			if (!enemy->isAlive()) continue;
			float enx = enemy->x - Hero.x;
			float eny = enemy->y - Hero.y;
			float dist = enx * enx + eny * eny;
			if (dist - (spellRadius + 100) * (spellRadius + 100) < 100) {
				// 造成伤害
				enemy->hp -= 1;
				if (enemy->type == eType::boss) {
					if (enemy->isAlive()) {
						continue;
					}
				}
				enemy->alive = false;
				Game::Audio.play(L"break");
				Game::Effects.spawn(EffectType::EXPLOSION, enemy->x, enemy->y);
				if (enemy->type == eType::boss) {
					Game::BG.setMode(BGMode::NORMAL);
					Game::Effects.clearSpellName();
				}
			}
		}
	}
}

void Game::updateBoss() {
	Boss* boss = nullptr;
	for (auto* e : E.getList()) {
		if (e == nullptr) continue;
		if (e->type == eType::boss) {
			boss = static_cast<Boss*>(e);
			break;
		}
	}
	if (!boss) {
		if (BG.getMode() == BGMode::BOSS_SPELL) {
			BG.setMode(BGMode::NORMAL); // 战斗结束切回普通背景
		}
		return;
	}
	HWND hwnd = GetHWnd();
	if (hwnd == nullptr || !IsWindow(hwnd)) return;
	if (boss->y < 0) return;
	// UI 绘制部分
	int currentHp = boss->hp;
	int maxPhHp = boss->getPhaseMaxHp();
	bool isSpell = boss->isSpellCardState();
	int timeLeft = boss->getPhaseTimeLeft();

	// 绘制 Boss 血条 
	int barW = 400; // 血条总宽
	int barH = 10;  // 血条高度
	int barX = (WIDTH - barW) / 2; 
	int barY = 30;  // 距离顶部 30 

	// 绘制背景槽 (灰色)
	setfillcolor(RGB(50, 50, 50));
	solidrectangle(barX, barY, barX + barW, barY + barH);
	// 绘制当前血量 (非符:白色, 符卡:红色)
	if (maxPhHp > 0) {
		float ratio = (float)currentHp / maxPhHp;
		if (ratio < 0) ratio = 0;
		if (ratio > 1) ratio = 1;
		setfillcolor(isSpell ? RGB(200, 50, 50) : WHITE);
		solidrectangle(barX, barY, barX + (int)(barW * ratio), barY + barH);
	}
	// 绘制血条外框 
	setlinecolor(WHITE);
	rectangle(barX, barY, barX + barW, barY + barH);

	// 绘制数字计时器 
	settextstyle(24, 0, L"Arial");
	// 倒计时少于 10 秒变红，否则白色
	if (timeLeft <= 10) settextcolor(RED);
	else settextcolor(BLUE);
	wchar_t timeStr[16];
	swprintf_s(timeStr, L"%02d", timeLeft);
	outtextxy(WIDTH - 50, 25, timeStr);
	settextcolor(WHITE);
}

void Game::CastSpellCard() {
	// 防止重复释放
	if (isSpellActive) return;

	// 清理可能存在的残留
	ClearSpellBarrages();

	isSpellActive = true;
	spellRadius = 10.0f; // 初始半径
	spellAngle = 0.0f;  // 初始角度
	Game::Audio.play(L"spell");
	Game::Effects.spawn(EffectType::SPELL_CUTIN, 0, 0, true);

	// 只生成 4 个弹幕
	for (int i = 0; i < 4; ++i) {
		Barrage* b = new Barrage(Hero.x, Hero.y);
		b->isFriendly = true; // 标记为友军
		b->alive = true;
		spellBarrages.push_back(b);
	}

	// 开启无敌
	Hero.invincible = true;
	// 设置一个足够长的保底时间，实际结束时间由弹幕飞出屏幕决定
	Hero.invincibleEnd = GetTickCount() + 10000;
}

void Game::UpdateSpellCard() {
	if (!isSpellActive) return;

	// 更新运动参数
	spellRadius += 3.5f; // 半径扩大速度 
	spellAngle += 0.1f;  // 旋转角速度

	float heroCx = Hero.x;
	float heroCy = Hero.y;
	bool allOut = true;

	// 更新每个的位置
	for (int i = 0; i < spellBarrages.size(); ++i) {
		Barrage* b = spellBarrages[i];

		// 四个弹幕分别偏移 0, 90, 180, 270 度
		float currentB_Angle = spellAngle + i * (3.14159f / 2.0f);

		// x = 圆心x + 半径 * cos(角度)
		b->x = heroCx + spellRadius * cos(currentB_Angle) - b->getWidth() / 2;
		b->y = heroCy + spellRadius * sin(currentB_Angle) - b->getHeight() / 2;
		b->draw();

		// 检查是否还在屏幕内 (只要有一个在屏幕内，符卡就不算结束)
		if (b->x > -100 && b->x < WIDTH + 100 && b->y > -100 && b->y < HEIGHT + 100) {
			allOut = false;
		}
	}
	// 如果半径很大了，且所有弹幕都出界了
	if (allOut && spellRadius > WIDTH) {
		isSpellActive = false;
		Hero.invincible = false; 
		ClearSpellBarrages();    
	}
}

void Game::ClearSpellBarrages() {
	for (auto* b : spellBarrages) delete b;
	spellBarrages.clear();
}

void Game::DrawDebug() {
	if ((GetAsyncKeyState(VK_TAB) & 0x8000) == 0) return;

	// 绘制敌人中心和判定圈
	for (auto* e : E.getList()) {
		if (e == nullptr) continue;
		if (!e->isAlive()) continue;
		setlinecolor(RED);
		setlinestyle(PS_SOLID, 1);
		setfillcolor(RED);
		// 中心点
		fillcircle((int)e->x, (int)e->y, 3);
		// 判定圈
		setlinecolor(GREEN);
		circle((int)e->x, (int)e->y, (int)(e->type == eType::boss ? 35 : 12));
	}
}

void Game::adjustWindow() {
	HWND hwnd = GetHWnd();
	ImmAssociateContext(hwnd, NULL);
	int scrW = GetSystemMetrics(SM_CXSCREEN);
	int scrH = GetSystemMetrics(SM_CYSCREEN);
	LONG style = GetWindowLong(hwnd, GWL_STYLE);
	int w = screenWidth;
	int h = screenHeight;
	int x = (scrW - w) / 2;
	int y = (scrH - h) / 2;
	MoveWindow(hwnd, x, y, w, h, TRUE);
}

void Game::drawUI() {

	// 置文字样式和颜色
	settextstyle(40, 0, L"微软雅黑", 0, 0, FW_BOLD, false, false, false);
	settextcolor(WHITE);
	setbkmode(TRANSPARENT);

	int uiX = LeftEdge + WIDTH + 30;
	int uiY = 100;

	// 绘制残机
	settextcolor(RED);
	outtextxy(uiX, uiY, L"Player");
	wchar_t lifeStr[16];
	swprintf_s(lifeStr, L"★ × %d", Hero.getLives()); 
	settextcolor(RED); // 残机通常用粉色/红色
	outtextxy(uiX + 10, uiY + 40, lifeStr);

	// 绘制符卡
	uiY += 100;
	settextcolor(YELLOW);
	outtextxy(uiX, uiY, L"Spell");
	wchar_t bombStr[16];
	swprintf_s(bombStr, L"★ × %d", Hero.getBombs()); 
	settextcolor(YELLOW);
	outtextxy(uiX + 10, uiY + 40, bombStr);

}


	
// 主菜单相关方法 

void Game::DrawMainMenu() {
	BG.drawBackdropOnly();

	// 绘制标题
	/**/

	// 按钮参数
	const int btnWidth = 200;
	const int btnHeight = 50;
	const int btnX = (screenWidth - btnWidth) / 2;
	const int btn1Y = 300;
	const int btn2Y = 380;

	int mouseX = menuCursorX;
	int mouseY = menuCursorY;

	// 绘制按钮1：普通模式
	bool hover1 = (mouseX >= btnX && mouseX <= btnX + btnWidth &&
		mouseY >= btn1Y && mouseY <= btn1Y + btnHeight);
	setfillcolor(hover1 ? RGB(100, 150, 255) : RGB(50, 100, 200));
	solidrectangle(btnX, btn1Y, btnX + btnWidth, btn1Y + btnHeight);
	setlinecolor(WHITE);
	rectangle(btnX, btn1Y, btnX + btnWidth, btn1Y + btnHeight);

	settextstyle(30, 0, L"微软雅黑", 0, 0, FW_NORMAL, false, false, false);
	settextcolor(WHITE);
	wchar_t btn1Text[] = L"普通模式";
	int btn1TextWidth = textwidth(btn1Text);
	outtextxy(btnX + (btnWidth - btn1TextWidth) / 2, btn1Y + 10, btn1Text);

	setbkmode(TRANSPARENT);

	// 绘制按钮2：AI演示
	bool hover2 = (mouseX >= btnX && mouseX <= btnX + btnWidth &&
		mouseY >= btn2Y && mouseY <= btn2Y + btnHeight);
	setfillcolor(hover2 ? RGB(100, 200, 100) : RGB(50, 150, 50));
	solidrectangle(btnX, btn2Y, btnX + btnWidth, btn2Y + btnHeight);
	setlinecolor(WHITE);
	rectangle(btnX, btn2Y, btnX + btnWidth, btn2Y + btnHeight);

	settextcolor(WHITE);
	wchar_t btn2Text[] = L"AI 演示";
	int btn2TextWidth = textwidth(btn2Text);
	outtextxy(btnX + (btnWidth - btn2TextWidth) / 2, btn2Y + 10, btn2Text);
}

void Game::HandleMenuInput() {
	// 按钮参数（与DrawMainMenu保持一致）
	const int btnWidth = 200;
	const int btnHeight = 50;
	const int btnX = (screenWidth - btnWidth) / 2;
	const int btn1Y = 300;
	const int btn2Y = 380;

	ExMessage msg;
	while (peekmessage(&msg, EM_MOUSE)) {
		menuCursorX = msg.x;
		menuCursorY = msg.y;
		if (msg.message != WM_LBUTTONDOWN) {
			continue;
		}
		int mouseX = msg.x;
		int mouseY = msg.y;

		if (mouseX >= btnX && mouseX <= btnX + btnWidth &&
			mouseY >= btn1Y && mouseY <= btn1Y + btnHeight) {
			InitNormalPlay();
			currentState = GameState::NORMAL_PLAY;
			return;
		}

		if (mouseX >= btnX && mouseX <= btnX + btnWidth &&
			mouseY >= btn2Y && mouseY <= btn2Y + btnHeight) {
			InitAIDemo();
			currentState = GameState::AI_DEMO;
			return;
		}
	}
}

void Game::InitNormalPlay() {
	// 清空敌人
	E.clearEnemy();
	// 清空子弹列表
	for (auto* bullet : B.bulletList) {
		delete bullet;
	}
	B.bulletList.clear();
	// 清空弹幕
	Barr.clearBarrage();
	// 清空道具
	for (auto* item : items) {
		delete item;
	}
	items.clear();
	// 清空符卡弹幕
	ClearSpellBarrages();
	// 重置关卡队列
	while (!waveQueue.empty()) {
		waveQueue.pop();
	}
	currentWave.clear();
	InitNormalLevels();
	// 重置其他状态
	enemyFire = false;
	wait = false;
	isSpellActive = false;
	bulletLevel = 1;
	// 重新初始化背景和音效
	BG.init();
	Effects.init();
}

void Game::InitAIDemo() {
	// AI模式初始化：清空所有游戏对象
	E.clearEnemy();
	// 清空子弹列表
	for (auto* bullet : B.bulletList) {
		delete bullet;
	}
	B.bulletList.clear();
	// 清空弹幕
	Barr.clearBarrage();
	// 清空道具
	for (auto* item : items) {
		delete item;
	}
	items.clear();
	// 清空符卡弹幕
	ClearSpellBarrages();
	// 清空关卡队列
	while (!waveQueue.empty()) {
		waveQueue.pop();
	}
	currentWave.clear();
	InitAILevel();
	// 重置状态
	enemyFire = false;
	wait = false;
	isSpellActive = false;
	// 初始化背景
	BG.init();
	if (aiProcessHandle == NULL) {
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		// 启动 python ai_server.py 
		char cmd[] = "python ai_server.py";

		// CREATE_NEW_CONSOLE 会弹出一个新的控制台黑框显示 AI 日志
		// 如果你以后不想看黑框了，可以把 CREATE_NEW_CONSOLE 换成 CREATE_NO_WINDOW
		if (CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
			aiProcessHandle = pi.hProcess; // 保存进程句柄，以后用来关掉它
			CloseHandle(pi.hThread);
			std::cout << "[Game] 已自动启动 Python AI 服务器" << std::endl;

			// 给 Python 留出 1 秒钟的启动时间，防止 C++ 发包过快导致 10054 报错
			Sleep(1000);
		}
		else {
			std::cerr << "[Game] 启动 Python 服务器失败！" << std::endl;
		}
	}
	// 创建并初始化 AI 控制器
	if (!aiController) {
		aiController = new AIController();
	}
	// 初始化网络（监听端口 9090）
	if (!aiController->Init(9090)) {
		std::cerr << "[Game] AI 控制器初始化失败！" << std::endl;
	}
	// 设置避障参数（危险半径 80 像素，斥力权重 0.8，意图权重 0.2）
	aiController->SetAvoidanceParams(80.0f, 0.8f, 0.2f);
	std::cout << "[Game] AI 演示模式初始化完成" << std::endl;
}

void Game::HandleAIDemo() {
	// 绘制背景
	BG.update();
	BG.draw();

	// 与普通模式保持一致：沿用同一套刷怪/弹幕/碰撞/Boss 逻辑
	HandleRound();

	// 1手动接管无敌时间的解除(因为 AI 模式没有调用 Hero.move())
		if (Hero.invincible && GetTickCount() > Hero.invincibleEnd) {
			Hero.invincible = false;
		}

	// 如果 AI 把残机用光了，强制拉起，保证它可以永远演示下去
	if (!Hero.isAlive()) {
		Hero.alive = true;
	}

	// AI 控制自机移动（混合避障）
	if (aiController) {
		aiController->UpdateHeroControl(Hero, Barr.barrList);
	}

	// 绘制自机
	Hero.draw();

	// 自机子弹逻辑
	Bullets();

	// 更新特效
	Effects.update();
	Effects.draw();

	// 每隔一定帧数发送游戏状态到 Python
	static int frameCounter = 0;
	frameCounter++;
	if (frameCounter >= 10 && aiController) {  // 每 10 帧发送一次（约 6 次/秒）
		aiController->SendGameState(Hero, E.getList(), Barr.barrList);
		frameCounter = 0;
	}

	// 在左上角显示提示文字
	settextstyle(30, 0, L"微软雅黑", 0, 0, FW_BOLD, false, false, false);
	settextcolor(YELLOW);
	setbkmode(TRANSPARENT);
	outtextxy(50, 50, L"AI Demo Mode Running...");

	// 提示：按ESC返回主菜单
	settextstyle(20, 0, L"微软雅黑", 0, 0, FW_NORMAL, false, false, false);
	settextcolor(WHITE);
	outtextxy(50, 100, L"按 ESC 返回主菜单");

	// 检测ESC键返回主菜单
	if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
		// 清理 AI 控制器
		if (aiController) {
			aiController->Close();
			delete aiController;
			aiController = nullptr;
		}

		if (aiProcessHandle != NULL) {
			TerminateProcess(aiProcessHandle, 0); // 强制结束进程
			CloseHandle(aiProcessHandle);
			aiProcessHandle = NULL;
			std::cout << "[Game] 已自动关闭 Python AI 服务器" << std::endl;
		}

		currentState = GameState::MAIN_MENU;
		Sleep(200); 
	}
}
