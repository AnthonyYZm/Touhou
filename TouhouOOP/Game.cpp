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
	InitBarrageHandlers(); // 构建弹幕分发表

	// 初始化状态为主菜单
	currentState = GameState::MAIN_MENU;

	// 初始化 AI 控制器为空指针
	aiController = nullptr;
	rlTrainingServer = nullptr;
}


Game::~Game() {
	ClearSpellBarrages();
	ClearQueuedWaves();

	// 清理 AI 控制器
	if (aiController) {
		delete aiController;
		aiController = nullptr;
	}

	if (rlTrainingServer) {
		delete rlTrainingServer;
		rlTrainingServer = nullptr;
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

		case GameState::RL_TRAINING:
			HandleRLTraining();
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

namespace {
    SpawnEvent MakeStageSpawn(int startTime, int count, int interval, eType type, int hp,
        float startX, float startY, const MoveStrategy& movement,
        const std::vector<BarrageTask>& tasks = std::vector<BarrageTask>()) {
        SpawnEvent event;
        event.startTime = startTime;
        event.count = count;
        event.interval = interval;
        event.type = type;
        event.hp = hp;
        event.startX = startX;
        event.startY = startY;
        event.moveLogic = movement;
        event.initTasks = tasks;
        return event;
    }

    BarrageTask MakeHoldingTask(int type, int interval, float speed, float omega, int num,
        int r, int dir, float acc, int style, int startDelay = 0) {
        BarrageTask task(type, interval, speed, omega, num, r, dir, 0, 0, acc, 1, 0, style);
        task.requireHolding = true;
        task.startDelay = startDelay;
        return task;
    }
}

void Game::InitStageDemo() {
    waveData stage;
    stage.waveDelay = 1000;
    const float left = (float)LeftEdge;
    const float right = (float)Right;
    const float top = (float)TopEdge;
    const float centreX = LeftEdge + WIDTH * 0.5f;

    // 0-12s：双列开场，低速直线弹仅用于教学。
    const std::vector<BarrageTask> openingTasks = {
        BarrageTask((int)bType::down_st, 1650, 2.0f, 0.0f, 1, 0, 1, 0, 0, 0.0f, 1, 0, (int)BulletStyle::GREEN_)
    };
    stage.events.push_back(MakeStageSpawn(0, 8, 450, eType::normal, 1, left - 40.0f, top + 80.0f,
        Moves::BezierPass({ left - 40.0f, top + 80.0f }, { left + 80.0f, top + 110.0f },
            { centreX - 180.0f, top + 260.0f }, { left + WIDTH * 0.42f, Bottom + 80.0f }, 5200, true, 0.30f, 0.62f), openingTasks));
    stage.events.push_back(MakeStageSpawn(0, 8, 450, eType::normal, 1, right + 40.0f, top + 80.0f,
        Moves::BezierPass({ right + 40.0f, top + 80.0f }, { right - 80.0f, top + 110.0f },
            { centreX + 180.0f, top + 260.0f }, { left + WIDTH * 0.58f, Bottom + 80.0f }, 5200, true, 0.30f, 0.62f), openingTasks));

    // 12-27s：燕返交叉，仅在贝塞尔中段发射少量放射弹。
    const std::vector<BarrageTask> swallowTasks = {
        BarrageTask((int)bType::firework, 2500, 2.15f, 0.0f, 6, 0, 1, 0, 0, 0.0f, 1, 0, (int)BulletStyle::BLUE_)
    };
    stage.events.push_back(MakeStageSpawn(12000, 10, 500, eType::normal, 1, left - 45.0f, top + 150.0f,
        Moves::BezierPass({ left - 45.0f, top + 150.0f }, { left + 190.0f, top + 40.0f },
            { right - 190.0f, top + 340.0f }, { right + 70.0f, top + 220.0f }, 6200, true, 0.35f, 0.65f), swallowTasks));
    stage.events.push_back(MakeStageSpawn(12000, 10, 500, eType::normal, 1, right + 45.0f, top + 150.0f,
        Moves::BezierPass({ right + 45.0f, top + 150.0f }, { right - 190.0f, top + 40.0f },
            { left + 190.0f, top + 340.0f }, { left - 70.0f, top + 220.0f }, 6200, true, 0.35f, 0.65f), swallowTasks));

    // 27-42s：三点花阵，精英仅在停稳后发射，首发依次错开 0/300/600ms。
    BarrageTask flowerLeft = MakeHoldingTask((int)bType::firework, 1500, 2.25f, 0.0f, 8, 0, 1, 0.0f, (int)BulletStyle::BLUE_, 0);
    BarrageTask flowerCenterBlue = MakeHoldingTask((int)bType::firework, 1700, 2.1f, 0.0f, 7, 0, 1, 0.0f, (int)BulletStyle::BLUE_, 300);
    BarrageTask flowerCenterRed = MakeHoldingTask((int)bType::firework, 1700, 2.1f, 0.0f, 7, 0, 1, 0.0f, (int)BulletStyle::RED_, 600);
    BarrageTask flowerRight = MakeHoldingTask((int)bType::firework, 1500, 2.25f, 0.0f, 8, 0, 1, 0.0f, (int)BulletStyle::RED_, 300);
    stage.events.push_back(MakeStageSpawn(27000, 1, 0, eType::elf, 20, left + 90.0f, top - 50.0f,
        Moves::EnterHoldExit({ left + 90.0f, top - 50.0f }, { left + 160.0f, top + 175.0f }, 1800, 8500, 180.0f), { flowerLeft }));
    stage.events.push_back(MakeStageSpawn(27300, 1, 0, eType::elf, 20, centreX, top - 60.0f,
        Moves::EnterHoldExit({ centreX, top - 60.0f }, { centreX, top + 125.0f }, 1800, 8800, 190.0f), { flowerCenterBlue, flowerCenterRed }));
    stage.events.push_back(MakeStageSpawn(27600, 1, 0, eType::elf, 20, right - 90.0f, top - 50.0f,
        Moves::EnterHoldExit({ right - 90.0f, top - 50.0f }, { right - 160.0f, top + 175.0f }, 1800, 8500, 180.0f), { flowerRight }));

    // 42-57s：中央风廊。两列反相折返，中心始终没有生成源。
    const std::vector<BarrageTask> windLeftTasks = {
        BarrageTask((int)bType::down_st, 1350, 2.35f, 0.0f, 1, 0, 1, 0, 0, 0.0f, 1, 0, (int)BulletStyle::BLUE_)
    };
    const std::vector<BarrageTask> windRightTasks = {
        BarrageTask((int)bType::down_st, 1350, 2.35f, 0.0f, 1, 0, 1, 0, 0, 0.0f, 1, 0, (int)BulletStyle::RED_)
    };
    stage.events.push_back(MakeStageSpawn(42000, 11, 520, eType::normal, 1, left + 150.0f, top - 35.0f,
        Moves::ZigzagDive(left + 165.0f, top - 35.0f, 72.0f, 1700, 205.0f, 0.0f), windLeftTasks));
    stage.events.push_back(MakeStageSpawn(42000, 11, 520, eType::normal, 1, right - 150.0f, top - 35.0f,
        Moves::ZigzagDive(right - 165.0f, top - 35.0f, 72.0f, 1700, 205.0f, PI), windRightTasks));
    BarrageTask sideAim = MakeHoldingTask((int)bType::pincer_aim, 2600, 3.3f, 0.0f, 1, 34, 1, 0.0f, (int)BulletStyle::GREEN_, 500);
    stage.events.push_back(MakeStageSpawn(44700, 1, 0, eType::elf, 28, left - 40.0f, top + 190.0f,
        Moves::EnterHoldExit({ left - 40.0f, top + 190.0f }, { left + 78.0f, top + 205.0f }, 1400, 7200, 190.0f), { sideAim }));
    stage.events.push_back(MakeStageSpawn(45100, 1, 0, eType::elf, 28, right + 40.0f, top + 190.0f,
        Moves::EnterHoldExit({ right + 40.0f, top + 190.0f }, { right - 78.0f, top + 205.0f }, 1400, 7200, 190.0f), { sideAim }));

    // 57-72s：环祭编队，八个护卫只移动，中央精英停稳后低密度发射。
    BarrageTask ritualRing = MakeHoldingTask((int)bType::circle_mill, 3000, 2.1f, 0.0f, 12, 72, 1, 0.0f, (int)BulletStyle::GREEN_, 300);
    BarrageTask ritualAim = MakeHoldingTask((int)bType::pincer_aim, 3600, 3.1f, 0.0f, 1, 46, 1, 0.0f, (int)BulletStyle::RED_, 1000);
    stage.events.push_back(MakeStageSpawn(57000, 1, 0, eType::elf, 30, centreX, top - 60.0f,
        Moves::EnterHoldExit({ centreX, top - 60.0f }, { centreX, top + 145.0f }, 1700, 9000, 180.0f), { ritualRing, ritualAim }));
    for (int i = 0; i < 8; ++i) {
        const float angle = i * (2.0f * PI / 8.0f);
        stage.events.push_back(MakeStageSpawn(57400, 1, 0, eType::normal, 7,
            centreX + 150.0f * cos(angle), top + 150.0f + 70.0f * sin(angle),
            Moves::OrbitThenExit({ centreX, top + 155.0f }, 150.0f, 70.0f, 7000, angle, (i % 2) == 0)));
    }

    // 72-87s：蛇行突击，左右入口交替，每只仅有一到两次自机狙机会。
    const std::vector<BarrageTask> snakeAim = {
        BarrageTask((int)bType::pincer_aim, 2500, 3.4f, 0.0f, 1, 0, 1, 0, 0, 0.0f, 1, 0, (int)BulletStyle::GREEN_)
    };
    stage.events.push_back(MakeStageSpawn(72000, 15, 1000, eType::normal, 1, left + 105.0f, top - 40.0f,
        Moves::ZigzagDive(left + 125.0f, top - 40.0f, 95.0f, 1500, 235.0f, 0.0f), snakeAim));
    stage.events.push_back(MakeStageSpawn(72500, 15, 1000, eType::normal, 1, right - 105.0f, top - 40.0f,
        Moves::ZigzagDive(right - 125.0f, top - 40.0f, 95.0f, 1500, 235.0f, PI), snakeAim));

    // 87-103s：镜门夹击，四只精英分两批镜像停稳后发射低密度曲线弹。
    BarrageTask blueGate = MakeHoldingTask((int)bType::curved_petal, 2800, 2.0f, 9.0f, 6, 720, 1, 0.007f, (int)BulletStyle::BLUE_, 0);
    BarrageTask redGate = MakeHoldingTask((int)bType::curved_petal, 2800, 2.0f, 9.0f, 6, 720, -1, 0.007f, (int)BulletStyle::RED_, 300);
    stage.events.push_back(MakeStageSpawn(87000, 1, 0, eType::elf, 35, left - 50.0f, top + 140.0f,
        Moves::EnterHoldExit({ left - 50.0f, top + 140.0f }, { left + 125.0f, top + 155.0f }, 1500, 7600, 185.0f), { blueGate }));
    stage.events.push_back(MakeStageSpawn(87000, 1, 0, eType::elf, 35, right + 50.0f, top + 140.0f,
        Moves::EnterHoldExit({ right + 50.0f, top + 140.0f }, { right - 125.0f, top + 155.0f }, 1500, 7600, 185.0f), { redGate }));
    stage.events.push_back(MakeStageSpawn(87700, 1, 0, eType::elf, 35, left - 50.0f, top + 215.0f,
        Moves::EnterHoldExit({ left - 50.0f, top + 215.0f }, { left + 190.0f, top + 215.0f }, 1500, 7200, 190.0f), { blueGate }));
    stage.events.push_back(MakeStageSpawn(87700, 1, 0, eType::elf, 35, right + 50.0f, top + 215.0f,
        Moves::EnterHoldExit({ right + 50.0f, top + 215.0f }, { right - 190.0f, top + 215.0f }, 1500, 7200, 190.0f), { redGate }));

    // 103-116s：万华终幕。FigureEight 每 3 秒切换 segment，二者交换曲线/环绕弹。
    BarrageTask figurePetal = MakeHoldingTask((int)bType::curved_petal, 2500, 2.05f, 10.0f, 7, 760, 1, 0.008f, (int)BulletStyle::BLUE_, 0);
    figurePetal.requireHolding = false;
    figurePetal.requiredMotionSegment = 0;
    BarrageTask figureOrbit = MakeHoldingTask((int)bType::orbit_release, 3200, 2.55f, 0.042f, 12, 760, -1, 0.16f, (int)BulletStyle::RED_, 0);
    figureOrbit.requireHolding = false;
    figureOrbit.requiredMotionSegment = 1;
    stage.events.push_back(MakeStageSpawn(103000, 1, 0, eType::elf, 30, centreX - 125.0f, top + 160.0f,
        Moves::FigureEight({ centreX - 115.0f, top + 175.0f }, 105.0f, 48.0f, 5200, 0.0f, 11000, 210.0f), { figurePetal, figureOrbit }));
    figurePetal.style = (int)BulletStyle::RED_;
    figurePetal.requiredMotionSegment = 1;
    figureOrbit.style = (int)BulletStyle::BLUE_;
    figureOrbit.requiredMotionSegment = 0;
    stage.events.push_back(MakeStageSpawn(103000, 1, 0, eType::elf, 48, centreX + 125.0f, top + 160.0f,
        Moves::FigureEight({ centreX + 115.0f, top + 175.0f }, 105.0f, 48.0f, 5200, PI, 11000, 210.0f), { figurePetal, figureOrbit }));
    stage.events.push_back(MakeStageSpawn(105000, 4, 550, eType::normal, 1, left - 35.0f, top + 90.0f,
        Moves::BezierPass({ left - 35.0f, top + 90.0f }, { left + 65.0f, top + 35.0f },
            { left + 80.0f, Bottom - 80.0f }, { left - 80.0f, Bottom + 30.0f }, 6000)));
    stage.events.push_back(MakeStageSpawn(105000, 4, 550, eType::normal, 1, right + 35.0f, top + 90.0f,
        Moves::BezierPass({ right + 35.0f, top + 90.0f }, { right - 65.0f, top + 35.0f },
            { right - 80.0f, Bottom - 80.0f }, { right + 80.0f, Bottom + 30.0f }, 6000)));

    // 116-120s：停止生成，并将仍在场的非 Boss 敌人切换为自然离场状态。
    SpawnEvent clearSignal;
    clearSignal.startTime = 116000;
    clearSignal.forceExitExisting = true;
    stage.events.push_back(clearSignal);

    waveQueue.push(stage);
}

void Game::InitNormalLevels() {
    InitStageDemo();
    AddSanaeBossWave(false);
}
void Game::InitAILevel() {
    AddSanaeBossWave(true);
}

void Game::AddSanaeBossWave(bool aiMode) {
    waveData waveBoss;
    waveBoss.waveDelay = 2000;

    SpawnEvent bossEvent;
    bossEvent.startTime = 500;
    bossEvent.count = 1;
    bossEvent.interval = 0;
    bossEvent.type = eType::boss;
    bossEvent.bossInstance = CreateSanaeBoss(aiMode);
    waveBoss.events.push_back(bossEvent);
    waveQueue.push(waveBoss);
}

void Game::ClearQueuedWaves() {
    // waveQueue 尚未交给 EnemyManager 的 Boss 由这里拥有；模式切换时必须一并释放。
    std::set<Boss*> queuedBosses;
    while (!waveQueue.empty()) {
        waveData wave = waveQueue.front();
        waveQueue.pop();
        for (const auto& event : wave.events) {
            if (event.bossInstance != nullptr) queuedBosses.insert(event.bossInstance);
        }
    }
    for (Boss* boss : queuedBosses) delete boss;
    nextWave = waveData();
}

Boss* Game::CreateSanaeBoss(bool aiMode) {
    // aiMode 只决定入口；普通模式和 AI 演示故意装配完全相同的阶段。
    (void)aiMode;
    Boss* sanae = new Boss((float)LeftEdge, (float)TopEdge);
    sanae->type = eType::boss;
    sanae->onEvent = [this](float x, float y, int id) {
        if (id == 1) {
            Audio.play(L"spell");
            Effects.spawn(EffectType::SPELL_CUTIN, 0, 0, false);
            Effects.spawn(EffectType::SPELL_NAME, 0, 0, false);
            BG.setMode(BGMode::BOSS_SPELL);
            Barr.clearBarrage();
        }
        else if (id == 2) {
            Audio.play(L"damage");
            Effects.spawn(EffectType::EXPLOSION, x, y);
            BG.setMode(BGMode::NORMAL);
            E.dropQueue.push_back({ x, y, 10 });
            Effects.clearSpellName();
            Barr.clearBarrage();
        }
    };
    AddSanaeBasePhases(*sanae);
    AddSanaeAdvancedPhases(*sanae);
    return sanae;
}

void Game::AddSanaeBasePhases(Boss& sanae) {
    BossPhase p1(600, 30000, Moves::bossEnter(80.0f), false, 1, 2000);
    p1.tasks.push_back(BarrageTask((int)bType::star_fall, 5000, 0.3f, 162.0f, 5,
        1500, 1, 100, 0, 0.02f, 40, 15, (int)BulletStyle::BLUE_));
    sanae.addPhase(p1);

    BossPhase p2(600, 45000, Moves::Stay(), true, 1000);
    BarrageTask leftWindmill((int)bType::windmill_st, 40, 3.0f, 20.3f, 1,
        0, 1, 0, 0, 0, 1, 0, (int)BulletStyle::RICE_BULE);
    leftWindmill.anchorMode = EmitterAnchor::ENEMY_OFFSET;
    leftWindmill.offsetX = -200;
    BarrageTask rightWindmill((int)bType::windmill_st, 40, 3.0f, 20.3f, 1,
        0, -1, 0, 0, 0, 1, 0, (int)BulletStyle::RICE_BULE);
    rightWindmill.anchorMode = EmitterAnchor::ENEMY_OFFSET;
    rightWindmill.offsetX = 200;
    p2.tasks.push_back(leftWindmill);
    p2.tasks.push_back(rightWindmill);
    p2.tasks.push_back(BarrageTask((int)bType::pincer_aim, 800, 4.0f, 0.0f, 1,
        150, 1, 0, 0, 0.0f, 1, 0, (int)BulletStyle::RED_));
    sanae.addPhase(p2);

    BossPhase p3(600, 45000, Moves::BurstWaypointPatrol({
        { LeftEdge + WIDTH * 0.50f, TopEdge + 120.0f },
        { LeftEdge + WIDTH * 0.24f, TopEdge + 185.0f },
        { LeftEdge + WIDTH * 0.76f, TopEdge + 185.0f }
        }, 1700, 900, true), false, 0);
    p3.tasks.push_back(BarrageTask((int)bType::windmill_switching, 100, 3.0f, 8.0f, 8,
        200, 1, 0, 0, 0.0f, 0, 1000, (int)BulletStyle::RICE_BULE));
    p3.tasks.push_back(BarrageTask((int)bType::windmill_switching, 100, 3.0f, 8.0f, 8,
        200, -1, 0, 0, 0.0f, 0, 1000, (int)BulletStyle::RICE_RED));
    sanae.addPhase(p3);

    BossPhase p4(600, 45000, Moves::MoveTo(LeftEdge + WIDTH * 0.5f, TopEdge + 180.0f, 80.0f), true, 1000);
    p4.tasks.push_back(BarrageTask((int)bType::firework, 500, 3.0f, 0.0f, 24,
        0, 1, 0, 0, 0.0f, 1, 0, (int)BulletStyle::RICE_BULE));
    p4.tasks.push_back(BarrageTask((int)bType::pincer_aim, 1500, 4.0f, 0.0f, 2,
        150, 1, 0, 0, 0.5f, 1, 80, (int)BulletStyle::RED_));
    sanae.addPhase(p4);

    BossPhase p5(600, 60000, Moves::FlowerPath(
        { LeftEdge + WIDTH * 0.5f, TopEdge + 165.0f }, 205.0f, 52.0f, 9000, 5), false, 1000);
    p5.tasks.push_back(BarrageTask((int)bType::star_fall, 5000, 0.5f, 370.0f, 12,
        1500, 1, 100, 0, 0.02f, 40, 15, (int)BulletStyle::RED_));
    p5.tasks.push_back(BarrageTask((int)bType::star_fall, 5000, 0.3f, 162.0f, 5,
        1500, 1, 100, 0, 0.02f, 40, 15, (int)BulletStyle::BLUE_));
    sanae.addPhase(p5);

    BossPhase p6(800, 45000, Moves::EllipsePatrol(
        { LeftEdge + WIDTH * 0.5f, TopEdge + 160.0f }, 175.0f, 48.0f, 7600, PI, false), true, 1000);
    p6.tasks.push_back(BarrageTask((int)bType::windmill, 350, 1.5f, 8.0f, 6,
        0, 1, 0, 0, 0.0f, 1, 0, (int)BulletStyle::BLUE_BIG));
    p6.tasks.push_back(BarrageTask((int)bType::windmill_st, 100, 3.0f, 15.0f, 12,
        0, 1, 0, 0, 0.0f, 1, 0, (int)BulletStyle::GREEN_));
    sanae.addPhase(p6);
}

void Game::AddSanaeAdvancedPhases(Boss& sanae) {
    // Phase 7：风符「双旋翠岚」，两组独立 task.currentAngle 的曲线花瓣。
    BossPhase p7(750, 42000,
        [](Enemy* enemy, int timePassed) {
            enemy->x = CentralX + 150.0f * sin(timePassed * 0.00135f);
            enemy->y = TopEdge + 165.0f;
            enemy->motionState = MotionState::MOVING;
            enemy->fire = true;
        }, false, 0);
    BarrageTask bluePetal((int)bType::curved_petal, 2300, 2.45f, 11.0f, 10,
        950, 1, 0, 0, 0.0105f, 6, 85, (int)BulletStyle::BLUE_);
    bluePetal.currentAngle = -11.0f; // 第一次发射从 0 度开始。
    BarrageTask redPetal((int)bType::curved_petal, 2300, 2.45f, 11.0f, 10,
        950, -1, 0, 0, 0.0105f, 6, 85, (int)BulletStyle::RED_);
    redPetal.currentAngle = 29.0f;   // 更新 -11 度后首圈为 +18 度偏移。
    p7.tasks.push_back(bluePetal);
    p7.tasks.push_back(redPetal);
    // 低频镜像风墙只作为阶段提示；左右内沿始终由 Barrage 保留 42px 安全通道。
    BarrageTask windWall((int)bType::mirror_sine_wall, 5200, 2.25f, 0.058f, 10,
        88, 1, 0, 0, 0.32f, 1, 0, (int)BulletStyle::GREEN_);
    windWall.startDelay = 1150;
    p7.tasks.push_back(windWall);
    sanae.addPhase(p7);

    // Phase 8：奇迹「星环再定向」，每颗只保存一次自机坐标快照。
    BossPhase p8(800, 45000, Moves::MoveTo(LeftEdge + WIDTH * 0.5f, TopEdge + 150, 12.0f), true, 1001);
    BarrageTask brakeRing((int)bType::brake_aim_ring, 3400, 3.1f, 7.5f, 30,
        950, 1, 0, 0, 0.075f, 3, 170, (int)BulletStyle::BLUE_);
    brakeRing.requireHolding = true;
    p8.tasks.push_back(brakeRing);
    sanae.addPhase(p8);

    // Phase 9：秘术「万华天穹」。红环 700ms、花瓣 1100ms 后启动，和蓝环错开。
    BossPhase p9(1000, 55000, Moves::FlowerPath(
        { LeftEdge + WIDTH * 0.5f, TopEdge + 165.0f }, 185.0f, 45.0f, 7600, 4, PI * 0.5f), true, 1002);
    BarrageTask blueOrbit((int)bType::orbit_release, 3000, 3.15f, 0.045f, 32,
        900, 1, 0, 0, 0.20f, 2, 150, (int)BulletStyle::BLUE_);
    BarrageTask redOrbit((int)bType::orbit_release, 3400, 2.85f, 0.040f, 24,
        1100, -1, 0, 0, 0.16f, 2, 180, (int)BulletStyle::RED_);
    redOrbit.startDelay = 700;
    BarrageTask finishingPetal((int)bType::curved_petal, 4700, 2.15f, 9.0f, 8,
        800, 1, 0, 0, 0.0080f, 3, 110, (int)BulletStyle::GREEN_);
    finishingPetal.currentAngle = -9.0f;
    finishingPetal.startDelay = 1100;
    p9.tasks.push_back(blueOrbit);
    p9.tasks.push_back(redOrbit);
    p9.tasks.push_back(finishingPetal);
    sanae.addPhase(p9);
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

	if (GetAsyncKeyState('Z') & 0x8000) { B.setFire(true); }

	// AI 开火
	if (Hero.fire) { B.setFire(true); }
	B.createBullet(&Hero, bulletLevel, E.getList());
}

void Game::InitBarrageHandlers() {
	// 表驱动法核心：将 bType 枚举值映射到对应的 Lambda 闭包
	// 新增弹幕类型时，只需在此处追加一条记录，Barrages() 主循环无需任何改动（符合开闭原则）
	const auto advanceWindmillAngle = [](BarrageTask& task) {
		// 保留旧版每波 +1.2° 的错位，同时加入小范围抖动，打破多悬臂的规则空隙。
		// 状态存放在 task 内，因此两个 Boss、颜色或阶段互不影响，也不依赖全局 rand()。
		task.jitterState = task.jitterState * 1664525u + 1013904223u;
		const float unit = (float)((task.jitterState >> 8) & 0xFFFFu) / 65535.0f;
		const float jitterDegrees = (unit * 2.0f - 1.0f) * 0.55f;
		task.currentAngle += (task.omega + 1.2f + jitterDegrees) * task.dir;
	};

	barrageHandlers[bType::down_st] = [this](BarrageTask& task, Enemy& en, int x, int y, int cx, int cy, BulletStyle style, float currentSpeed, DWORD now) {
		Barr.Normal(en, task.speed, style);
	};

	// windmill_st 通过 task 引用直接修改 currentAngle，保证状态在连发间正确累积
	barrageHandlers[bType::windmill_st] = [this, advanceWindmillAngle](BarrageTask& task, Enemy& en, int x, int y, int cx, int cy, BulletStyle style, float currentSpeed, DWORD now) {
		advanceWindmillAngle(task);
		Barr.straightMill(en, task.speed, task.currentAngle, task.num, x, y, 1, style);
	};

	barrageHandlers[bType::firework] = [this](BarrageTask& task, Enemy& en, int x, int y, int cx, int cy, BulletStyle style, float currentSpeed, DWORD now) {
		Barr.fireWork(en, currentSpeed, task.num, x, y, style);
	};

	barrageHandlers[bType::circle_mill] = [this](BarrageTask& task, Enemy& en, int x, int y, int cx, int cy, BulletStyle style, float currentSpeed, DWORD now) {
		Barr.circleMill(en, task.speed, task.r, task.num, x, y, style);
	};

	barrageHandlers[bType::windmill] = [this](BarrageTask& task, Enemy& en, int x, int y, int cx, int cy, BulletStyle style, float currentSpeed, DWORD now) {
		task.currentAngle += task.omega * task.dir;
		Barr.straightMill2(en, task.speed, task.currentAngle, task.num, x, y, 1, style);
	};

	// pincer_aim 通过捕获 this 访问 Hero 的实时坐标
	barrageHandlers[bType::pincer_aim] = [this](BarrageTask& task, Enemy& en, int x, int y, int cx, int cy, BulletStyle style, float currentSpeed, DWORD now) {
		Barr.pincerAim(en, Hero.x, Hero.y, currentSpeed, task.r, task.num, x, y, style);
	};

	barrageHandlers[bType::random_rain] = [this](BarrageTask& task, Enemy& en, int x, int y, int cx, int cy, BulletStyle style, float currentSpeed, DWORD now) {
		Barr.randomRain(currentSpeed, style);
	};

	// star_fall 需要 now 计算释放时间，cx/cy 作为爆炸中心点
	barrageHandlers[bType::star_fall] = [this](BarrageTask& task, Enemy& en, int x, int y, int cx, int cy, BulletStyle style, float currentSpeed, DWORD now) {
		DWORD releaseTime = task.lastTime + task.r;
		if (releaseTime < now) releaseTime = now;
		Barr.fiveStar(en, task.currentBurst, task.burstCount, task.omega,
			(float)task.x0, task.speed, task.acc, releaseTime, task.num, cx, cy, style);
	};

	// 曲线花瓣的基础角由各自任务保存，蓝/红两组以及不同 Boss 不会共享旋转状态。
	barrageHandlers[bType::curved_petal] = [this](BarrageTask& task, Enemy& en, int x, int y, int cx, int cy, BulletStyle style, float currentSpeed, DWORD now) {
		task.currentAngle += task.omega * task.dir;
		Barr.curvedPetal(en, task.speed, task.currentAngle, task.num, (DWORD)task.r, task.dir, task.acc, x, y, style);
	};

	barrageHandlers[bType::brake_aim_ring] = [this](BarrageTask& task, Enemy& en, int x, int y, int cx, int cy, BulletStyle style, float currentSpeed, DWORD now) {
		task.currentAngle += task.omega * task.dir;
		// 仅在发射这一刻读取 Hero，子弹内部保存快照后不再追踪。
		Barr.brakeAimRing(en, Hero.x, Hero.y, task.speed, task.currentAngle, task.num,
			(DWORD)task.r, task.acc, x, y, style);
	};

	barrageHandlers[bType::mirror_sine_wall] = [this](BarrageTask& task, Enemy& en, int x, int y, int cx, int cy, BulletStyle style, float currentSpeed, DWORD now) {
		Barr.mirrorSineWall(en, task.speed, task.omega, task.num, (float)task.r, task.dir, task.acc, x, y, style);
	};

	barrageHandlers[bType::orbit_release] = [this](BarrageTask& task, Enemy& en, int x, int y, int cx, int cy, BulletStyle style, float currentSpeed, DWORD now) {
		// cx/cy 为此波生成时的 Boss 中心快照，后续 Boss 移动不会拖动环绕弹。
		Barr.orbitRelease(en, task.speed, task.omega, task.num, (DWORD)task.r, task.dir, task.acc, cx, cy, style);
	};
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
			if (task.requireHolding && en->motionState != MotionState::HOLDING) continue;
			if (task.requiredMotionSegment >= 0 && en->motionSegment != task.requiredMotionSegment) continue;

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
					if ((DWORD)(now - task.lastBurstTime) >= (DWORD)task.r) {
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

			const bType taskType = (bType)task.type;
			const bool motionUsesAcc = taskType == bType::curved_petal ||
				taskType == bType::brake_aim_ring || taskType == bType::mirror_sine_wall ||
				taskType == bType::orbit_release;
			float currentSpeed = motionUsesAcc ? task.speed : task.speed - (task.currentBurst * task.acc);
			if (currentSpeed < 0.5f) currentSpeed = 0.5f;

			int x = centerX;
			int y = centerY;
			switch (task.anchorMode) {
			case EmitterAnchor::ENEMY_OFFSET:
				x += task.offsetX;
				y += task.offsetY;
				break;
			case EmitterAnchor::SCREEN_ABSOLUTE:
				x = task.x0;
				y = task.y0;
				break;
			case EmitterAnchor::ENEMY_CENTER:
			default:
				break;
			}

			// 表驱动分发：O(1) 查表，找到对应 handler 并执行
			auto handlerIt = barrageHandlers.find((bType)task.type);
			if (handlerIt != barrageHandlers.end()) {
				const int originalDir = task.dir;
				if (task.invertDirectionBySegment && (en->motionSegment & 1) != 0) task.dir = -task.dir;
				handlerIt->second(task, *en, x, y, centerX, centerY, style, currentSpeed, now);
				task.dir = originalDir;
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
	const int btn3Y = 460;

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

	// 绘制按钮3：RL训练
	bool hover3 = (mouseX >= btnX && mouseX <= btnX + btnWidth &&
		mouseY >= btn3Y && mouseY <= btn3Y + btnHeight);
	setfillcolor(hover3 ? RGB(220, 180, 80) : RGB(170, 120, 40));
	solidrectangle(btnX, btn3Y, btnX + btnWidth, btn3Y + btnHeight);
	setlinecolor(WHITE);
	rectangle(btnX, btn3Y, btnX + btnWidth, btn3Y + btnHeight);

	settextcolor(WHITE);
	wchar_t btn3Text[] = L"RL 训练";
	int btn3TextWidth = textwidth(btn3Text);
	outtextxy(btnX + (btnWidth - btn3TextWidth) / 2, btn3Y + 10, btn3Text);
}

void Game::HandleMenuInput() {
	// 按钮参数（与DrawMainMenu保持一致）
	const int btnWidth = 200;
	const int btnHeight = 50;
	const int btnX = (screenWidth - btnWidth) / 2;
	const int btn1Y = 300;
	const int btn2Y = 380;
	const int btn3Y = 460;

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

		if (mouseX >= btnX && mouseX <= btnX + btnWidth &&
			mouseY >= btn3Y && mouseY <= btn3Y + btnHeight) {
			InitRLTraining();
			currentState = GameState::RL_TRAINING;
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
	ClearQueuedWaves();
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

void Game::InitRLTraining() {
	// 进入 RL 训练时清空普通关卡和 AI 演示遗留对象，只保留训练服务需要的场景。
	E.clearEnemy();
	for (auto* bullet : B.bulletList) {
		delete bullet;
	}
	B.bulletList.clear();
	Barr.clearBarrage();
	for (auto* item : items) {
		delete item;
	}
	items.clear();
	ClearSpellBarrages();
	ClearQueuedWaves();
	currentWave.clear();

	enemyFire = false;
	wait = false;
	isSpellActive = false;
	BG.init();
	Effects.init();

	if (aiController) {
		aiController->Close();
		delete aiController;
		aiController = nullptr;
	}
	if (aiProcessHandle != NULL) {
		TerminateProcess(aiProcessHandle, 0);
		CloseHandle(aiProcessHandle);
		aiProcessHandle = NULL;
	}
	if (rlTrainingServer) {
		delete rlTrainingServer;
		rlTrainingServer = nullptr;
	}

	// C++ 作为 Gym 环境服务端，Python 训练脚本通过 UDP 9090 请求 reset/step。
	rlTrainingServer = new RLTrainingServer(&Hero, &E, &Barr.barrList);
	if (!rlTrainingServer->Init(9090)) {
		std::cerr << "[Game] RL training server failed to start" << std::endl;
	}
	else {
		std::cout << "[Game] RL training mode is ready" << std::endl;
	}
}

void Game::HandleRLTraining() {
	// 训练模式主循环只负责展示状态和维护窗口；环境推进由 RLTrainingServer 响应 Python 请求触发。
	BG.update();
	BG.draw();

	setbkmode(TRANSPARENT);
	settextstyle(30, 0, L"微软雅黑", 0, 0, FW_BOLD, false, false, false);
	settextcolor(YELLOW);
	outtextxy(50, 50, L"RL Training Mode");

	settextstyle(20, 0, L"微软雅黑", 0, 0, FW_NORMAL, false, false, false);
	settextcolor(WHITE);
	outtextxy(50, 95, L"UDP 9090 等待 Python reset/step");
	outtextxy(50, 125, L"运行 train_sterile.py 开始训练");
	outtextxy(50, 155, L"按 ESC 返回主菜单");

	if (!rlTrainingServer || !rlTrainingServer->IsRunning()) {
		settextcolor(RED);
		outtextxy(50, 190, L"训练服务未运行，检查端口是否被占用");
	}

	// ESC 退出时销毁训练服务，释放 9090 端口，避免下次进入时 bind 失败。
	if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
		if (rlTrainingServer) {
			delete rlTrainingServer;
			rlTrainingServer = nullptr;
		}
		currentState = GameState::MAIN_MENU;
		Sleep(200);
	}
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
	ClearQueuedWaves();
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
