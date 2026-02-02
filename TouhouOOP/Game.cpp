#include "Game.h"
#include "Library.h"


int Game::bulletLevel = 1;
AudioManager Game::Audio;
EffectManager Game::Effects;
BackgroundManager Game::BG;

Game::Game() {
	initgraph(WIDTH, HEIGHT);
	enemyFire = false;
	wait = false;	
	bulletLevel = 1;
	InitLevels();
	isSpellActive = false;
	spellAngle = 0.0f;
	spellRadius = 10.0f;	
	Audio.init();
	Effects.init();
	BG.init();
	
}


Game::~Game() {
	ClearSpellBarrages();
}

void Game::Touhou() {
	Scr.gameScreen();
	BeginBatchDraw();

	const int FPS = 60;
	const int FRAME_WAIT = 1000 / FPS;

	while (IsWindow(GetHWnd())) {
		DWORD startTime = GetTickCount();
		cleardevice();
		BG.update();
		BG.draw();
		//Game logic
		HandleRound();
		Effects.update();
		Effects.draw();
		HeroControl();
		Bullets();
		
		FlushBatchDraw();

		DWORD endTime = GetTickCount();
		DWORD frameTime = endTime - startTime; 
		if (frameTime < FRAME_WAIT) {
			Sleep(FRAME_WAIT - frameTime);
		}
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
		if (e->type == eType::boss && e->isAlive()) {
			isBossAlive = true;
			break;
		}
	}
	if (isBossAlive) {
		Game::Audio.playBGM(L"bgm_boss");
	}
	else {
		Game::Audio.playBGM(L"bgm_stage1");
	}
}

void Game::InitLevels() {
	
	{
		// 第一波
		waveData w1;
		w1.waveDelay = 2000; // 第一波开始前等待 2 秒
		SpawnEvent e1;
		e1.startTime = 500; // 波次开始后 500ms 生成
		e1.count = 15; 		// 生成 5 个敌人
		e1.interval = 400;  // 每 400ms 生成一个
		e1.type = eType::normal; // 普通敌人
		e1.startX = CentralX; e1.startY = TopEdge; // 起始位置
		e1.moveLogic = Moves::SineWave(CentralX, 50, 2.0f, 3.0f); // 设定移动逻辑
		//e1.initTasks.push_back(BarrageTask((int)bType::down_st, 500, 5.0f, 0, 1)); // 弹幕任务
		w1.events.push_back(e1); 
		waveQueue.push(w1);

		// 第二波
		waveData w2;
		w2.waveDelay = 2000;

		SpawnEvent e2;
		e2.startTime = 2000;
		e2.count = 1;
		e2.type = eType::elf;
		e2.hp = 20;
		e2.startX = CentralX + 200; e2.startY = TopEdge;
		e2.moveLogic = Moves::Hover(CentralY, 2.0f);
		e2.initTasks.push_back(BarrageTask((int)bType::windmill_st, 120, 4.0f, 5, 1));
		
		SpawnEvent e3;
		e3.startTime = 2000;
		e3.count = 1;
		e3.type = eType::elf;
		e3.hp = 20;
		e3.startX = CentralX - 200; e3.startY = TopEdge;
		e3.moveLogic = Moves::Hover(CentralY, 2.0f);
		e3.initTasks.push_back(BarrageTask((int)bType::windmill_st, 80, 4.0f, 3, 1));
		w2.events.push_back(e2);
		w2.events.push_back(e3);
		waveQueue.push(w2);

		// 第三波
		waveData w3;
		w3.waveDelay = 2000;
		
		SpawnEvent e4;
		e4.startTime = 2000;
		e4.count = 50;
		e4.type = eType::normal;
		e4.interval = 100;
		e4.startX = LeftEdge; e4.startY = CentralY - 50;
		e4.moveLogic = Moves::shuttle(10, 1);
		//e4.initTasks.push_back(BarrageTask((int)bType::down_st, 300, 2.0f, 0, 1));

		SpawnEvent e5;
		e5.startTime = 2000;
		e5.count = 50;
		e5.type = eType::normal;
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
		e6.startX = CentralX - 200; e6.startY = TopEdge; 
		e6.moveLogic = Moves::SineWave(CentralX - 200, 50, 2.0f, 3.0f);
		
		SpawnEvent e7;
		e7.startTime = 500;
		e7.count = 20;
		e7.interval = 400;
		e7.type = eType::normal;
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
				Game::Barr.clearBarrage(); // 消除旧子弹
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
		/* 添加五星崩解任务
		// 参数映射参考：
		// interval: 5000 (每5秒一轮)
		// speed: 2.0f (拉伸速度，控制线段变长的快慢)
		// acc: 0.15f (抛射加速度，控制星星炸开的力度，0.1~0.2 之间效果最好)
		// omega: 150.0f (大圆半径，5个星星离Boss多远)
		// num: 
		// r: 500 (悬停时间 500ms)
		// dir: 1 (未使用)
		// x0: 60 (星星半径，控制五角星大小)
		// y0: 0 (未使用)
		// burstCount: 100 (总共画100笔，越多越细腻)
		// burstInterval: 10 (每笔间隔10ms，作画总耗时 1秒)
		omega = 1.618 * */
		p1.tasks.push_back(BarrageTask(
			(int)bType::star_fall,      // type
			5000,                       // interval
			1.0,                       // speed (stretch)
			162.0f,                     // omega (Orbit Radius)
			5,                          // num
			1500,                        // r (Pause Time)
			1,                          // dir
			100,                         // x0 (Star Radius)
			0,                          // y0
			0.05f,                       // acc (Normal Acc)
			40,                        // burstCount
			15                         // burstInterval
		));
		sanae->addPhase(p1);
		}

		// phase2
		{
		BossPhase p2(
			800,
			45000,
			Moves::Stay(),
			true,
			1000
		);
		int left = CentralX - 200;
		int right = CentralX + 200;
		p2.tasks.push_back(BarrageTask((int)bType::windmill_st, 100, 3.0f, 10, 1, 0, 1, left, 0));
		p2.tasks.push_back(BarrageTask((int)bType::windmill_st, 100, 3.0f, 10, 1, 0, 0, right, 0));
		p2.tasks.push_back(BarrageTask((int)bType::pincer_aim, 800, 4.0f, 0, 1,
			150, 1, 0, 0, 0, 1));
		sanae->addPhase(p2);
		}

		// phase3
		BossPhase p3(
			800,   
			45000,  
			Moves::StepLeftUp(3000, 200, 1, 1),
			false,
			0
		);
		p3.tasks.push_back(BarrageTask(
			(int)bType::windmill_switching,
			100,             // [interval] 发射间隔 (越小线条越密集)
			3.0f,           // [speed] 子弹飞行速度
			8.0f,           // [omega] 旋转角速度 (每发旋转4度)
			8,              // [num] 几条旋臂 (4叶风车)
			200,              // 停顿
			1,              // [dir] 初始方向 (1:顺时针)
			0, 0,           // x0, y0
			0,              // burstCount (未使用)
			0,        // burstCount (unused)
			3000      // burstInterval (switch time, 3000ms)
		));
		sanae->addPhase(p3);
		// phase4
		{
		BossPhase p4(
			800,   // 血量更多
			45000,  // 限时 45秒
			Moves::MoveTo(CentralX, CentralY, 80.0f), // 移动逻辑：原地不动
			true  // 是符卡
		);
		
		p4.tasks.push_back(BarrageTask((int)bType::firework, 500, 3.0f, 0, 24));
		p4.tasks.push_back(BarrageTask((int)bType::pincer_aim, 1500, 4.0f, 0, 2, 
							150, 1, 0, 0, 0.5f, 3, 80));
		sanae->addPhase(p4);
		}

		// phase5
		{
		BossPhase p5(
			300,
			60000,
			Moves::Stay(),
			false,
			1000
		);
		p5.tasks.push_back(BarrageTask(
			(int)bType::star_fall,      // type
			5000,                       // interval
			0.5,                       // speed (stretch)
			370.0f,                     // omega (Orbit Radius)
			12,                          // num
			1500,                        // r (Pause Time)
			1,                          // dir
			100,                         // x0 (Star Radius)
			0,                          // y0
			0.05f,                       // acc (Normal Acc)
			40,                        // burstCount
			15                         // burstInterval
		));
		sanae->addPhase(p5);
		}
		
		// phase6
		{
		BossPhase p6(
			600,
			45000,
			Moves::Stay(),
			true,
			1000
		);
		p6.tasks.push_back(BarrageTask((int)bType::firework, 300, 3.0f, 0, 15));
		p6.tasks.push_back(BarrageTask((int)bType::windmill_st, 100, 3.0f, 10, 12));
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
	B.createBullet(&Hero, bulletLevel, E.getList());
}

void Game::Barrages() {
	DWORD now = GetTickCount();

	// 1. 遍历所有敌人
	for (auto* en : E.getList()) {
		if (en == nullptr) continue;
		if (!en->isAlive()) continue;
		if (!en->isFire()) continue;

		int centerX = (int)en->x;
		int centerY = (int)en->y;

		// 3. 执行任务
		for (auto& task : en->GetTasks()) {

			if (task.type == (int)bType::windmill_switching) {
				if (task.lastBurstTime == 0) {
					task.lastBurstTime = now;
					task.lastTime = now - task.interval;
				}
				if (task.currentBurst == 0) {
					// 这里的 interval 决定了发射密度
					if (now - task.lastTime >= task.interval) {
						task.lastTime = now; 
						task.currentAngle += task.omega * task.dir;
						Barr.directionalMill(*en, task.speed, task.currentAngle, task.num, centerX, centerY);
					}

					if (now - task.lastBurstTime >= task.burstInterval) {
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
				if (now - task.lastTime >= task.interval) {
					shouldFire = true;
					task.lastTime = now;
				}
			}
			// 连发
			else if (task.currentBurst > 0 && task.currentBurst < task.burstCount) {
				if (now - task.lastBurstTime >= task.burstInterval) {
					shouldFire = true;
				}
			}

			if (!shouldFire) continue;

			float currentSpeed = task.speed - (task.currentBurst * task.acc);
			if (currentSpeed < 0.5f) currentSpeed = 0.5f;

			int x = (task.x0 == 0) ? centerX : task.x0;
			int y = (task.y0 == 0) ? centerY : task.y0;

			switch ((bType)task.type) {
			case bType::down_st: Barr.Normal(*en, task.speed); break;
			case bType::windmill_st: Barr.straightMill(*en, task.speed, (int)task.omega, task.num, x, y, 1); break;
			case bType::firework: Barr.fireWork(*en, currentSpeed, task.num, x, y); break;
			case bType::circle_mill: Barr.circleMill(*en, task.speed, task.r, task.num, x, y); break;
			case bType::wheel: Barr.wheel(*en, task.speed, task.omega, task.num, x, y); break;
			case bType::pincer_aim: Barr.pincerAim(*en, Hero.x, Hero.y, currentSpeed, task.r, task.num, x, y); break;
			case bType::random_rain: Barr.randomRain(currentSpeed); break;
			case bType::star_fall: {
				DWORD releaseTime = task.lastTime + task.r;
				if (releaseTime < now) releaseTime = now;
				Barr.fiveStar(*en, task.currentBurst, task.burstCount, task.omega,
					(float)task.x0, task.speed, task.acc, releaseTime, task.num, centerX, centerY
				);
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
	// 1. 生成新道具 (从 EnemyManager 获取掉落请求)
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

	// 2. 移动与拾取
	float heroCx = Hero.x + Hero::getWidth() / 2;
	float heroCy = Hero.y + Hero::getHeight() / 2;
	//bool highPower = (Hero.y < HEIGHT / 3); // "收点线"：如果在屏幕上方 1/3，全屏吸附

	for (auto it = items.begin(); it != items.end(); ) {
		Item* item = *it;

		// 磁力判断
		// 距离吸附 (比如 100 像素内)
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

		// 拾取检测 (距离非常近)
		if (checkCircleCollide(item->x + 8, item->y + 8, 8, heroCx, heroCy, 16)) { // 16是自机拾取半径
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

	float heroR = (float)Hero.JudgeR;
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

			// --- A. 获取敌人判定参数 ---
			float eCx = enemy->x;
			float eCy = enemy->y;
			float enemyR = 12.0f;
			if (enemy->type == eType::elf) enemyR = 32.0f;
			else if (enemy->type == eType::boss) enemyR = 30.0f; // 大精灵18，小兵12
			// --- B. AABB 粗筛 (性能优化) ---
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
						Hero::addScore(1000); // 普通分

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
			float barrR = 6.0f; 

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
			if (!e->isAlive()) continue;

			float eCx = e->x;
			float eCy = e->y;
			float eR = (float)min(e->width, e->height) / 3; 

			if (checkCircleCollide(Hero.x, Hero.y, heroR, eCx, eCy, eR)) {
				Hero.hit();
				Game::Audio.play(L"dead");
				break;
			}
		}
	}

	// 符卡碰撞逻辑
	if (isSpellActive && !spellBarrages.empty()) {

		// 1. 符卡弹幕 vs 敌方弹幕
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
		
		// 2. 符卡弹幕 vs 敌人
		for (auto* enemy : E.getList()) {
			if (!enemy->isAlive()) continue;
			float enx = enemy->x - Hero.x;
			float eny = enemy->y - Hero.y;
			float dist = enx * enx + eny * eny;
			if (dist - (spellRadius + 100) * (spellRadius + 100) < 100) {
				// 造成伤害
				enemy->hp -= 0.05f;
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
		if (e->type == eType::boss) {
			boss = (Boss*)e; // 强制转换
			break;
		}
	}
	if (!boss) {
		if (BG.getMode() == BGMode::BOSS_SPELL) {
			BG.setMode(BGMode::NORMAL); // 战斗结束切回普通背景
		}
		return;
	}
	if (boss->y < 0) return;
	// UI 绘制部分
	int currentHp = boss->hp;
	int maxPhHp = boss->getPhaseMaxHp();
	bool isSpell = boss->isSpellCardState();
	int timeLeft = boss->getPhaseTimeLeft();

	// 绘制 Boss 血条 (屏幕正上方)
	int barW = 400; // 血条总宽
	int barH = 10;  // 血条高度
	int barX = (WIDTH - barW) / 2; // 居中
	int barY = 30;  // 距离顶部 30 像素

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

	// B. 绘制数字计时器 (屏幕右上角)
	settextstyle(24, 0, L"Arial");
	// 倒计时少于 10 秒变红，否则白色
	if (timeLeft <= 10) settextcolor(RED);
	else settextcolor(BLUE);
	wchar_t timeStr[16];
	swprintf_s(timeStr, L"%02d", timeLeft); // 格式化为两位数字
	// 绘制在右上角 (留出一点边距)
	outtextxy(WIDTH - 50, 25, timeStr);
	settextcolor(WHITE);
}

void Game::CastSpellCard() {
	// 防止重复释放
	if (isSpellActive) return;

	// 清理可能存在的残留
	ClearSpellBarrages();

	isSpellActive = true;
	spellRadius = 10.0f; // 初始半径为0（从自机身体里钻出来）
	spellAngle = 0.0f;  // 初始角度
	Game::Audio.play(L"spell");
	Game::Effects.spawn(EffectType::SPELL_CUTIN, 0, 0, true);

	// 只生成 4 个弹幕
	for (int i = 0; i < 4; ++i) {
		Barrage* b = new Barrage(Hero.x, Hero.y);
		b->isFriendly = true; // 标记为友军
		b->alive = true;
		// 这里不需要设置 vx/vy，因为我们会手动控制它的 x,y
		spellBarrages.push_back(b);
	}

	// 开启无敌
	Hero.invincible = true;
	// 设置一个足够长的保底时间，实际结束时间由弹幕飞出屏幕决定
	Hero.invincibleEnd = GetTickCount() + 10000;
}

void Game::UpdateSpellCard() {
	if (!isSpellActive) return;

	// 1. 更新运动参数
	spellRadius += 3.5f; // 半径扩大速度 (数值越大扩散越快)
	spellAngle += 0.1f;  // 旋转角速度

	// 获取自机中心点
	float heroCx = Hero.x;
	float heroCy = Hero.y;

	bool allOut = true; // 标记是否所有弹幕都飞出屏幕

	// 2. 更新每个弹幕的位置
	for (int i = 0; i < spellBarrages.size(); ++i) {
		Barrage* b = spellBarrages[i];

		// 计算当前弹幕的角度 (四个弹幕分别偏移 0, 90, 180, 270 度)
		float currentB_Angle = spellAngle + i * (3.14159f / 2.0f);

		// [核心算法] 圆周运动坐标公式
		// x = 圆心x + 半径 * cos(角度)
		b->x = heroCx + spellRadius * cos(currentB_Angle) - Barrage::getDarkGreenWidth() / 2;
		b->y = heroCy + spellRadius * sin(currentB_Angle) - Barrage::getDarkGreenHeight() / 2;

		// 绘制
		b->draw();

		// 检查是否还在屏幕内 (只要有一个在屏幕内，符卡就不算结束)
		// 稍微放宽一点边界判断 (-100)，保证完全飞出去了才算结束
		if (b->x > -100 && b->x < WIDTH + 100 && b->y > -100 && b->y < HEIGHT + 100) {
			allOut = false;
		}
	}

	// 3. 结束判定：如果半径很大了，且所有弹幕都出界了
	if (allOut && spellRadius > WIDTH) {
		isSpellActive = false;
		Hero.invincible = false; // 取消无敌
		ClearSpellBarrages();    // 清空对象
	}
}

void Game::ClearSpellBarrages() {
	for (auto* b : spellBarrages) delete b;
	spellBarrages.clear();
}

void Game::DrawDebug() {
	// 只有在按下 TAB 键时显示调试信息
	if ((GetAsyncKeyState(VK_TAB) & 0x8000) == 0) return;

	// 1. 绘制敌人中心和判定圈
	for (auto* e : E.getList()) {
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



	