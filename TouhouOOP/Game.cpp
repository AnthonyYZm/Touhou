#include "Game.h"

int Game::bulletLevel = 1;

Game::Game() {
	enemyFire = false;
	wait = false;	
	bulletLevel = 1;
	InitLevels();
	isSpellActive = false;
	spellAngle = 0.0f;
	spellRadius = 10.0f;	
}

Game::~Game() {
	ClearSpellBarrages();
}

void Game::Touhou() {
	Scr.gameScreen();
	BeginBatchDraw();

	while (IsWindow(GetHWnd())) {
		cleardevice();
		//Scr.gameBackground();
		Scr.gameBackground();
		//Game logic
		HandleRound();
		HeroControl();
		Bullets();
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
	}
	Barrages(); 
	Enemies();  
	UpdateItems(); 
	UpdateSpellCard();
	CheckCollision();
}

void Game::InitLevels() {
	// --- 第一波 (Wave 1) ---
	{
		waveData w1;
		w1.waveDelay = 2000; // 第一波开始前等待 2 秒（或者是上一波结束后的间隔）

		// 事件1配置...
		SpawnEvent e1;
		e1.startTime = 500; // 波次开始后 500ms 生成
		e1.count = 50; 		// 生成 5 个敌人
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
	// 自机子弹 vs 敌人 。
	float bulletR = 6.0f;

	// 获取引用，方便操作
	auto& enemies = E.getList();
	auto& bullets = B.bulletList;

	for (auto& b : bullets) {
		if (!b->isAlive()) continue;

		float bCx = b->x + Bullet::getBulletWidth() / 2.0f;
		float bCy = b->y + Bullet::getBulletHeight() / 2.0f;

		for (auto it = enemies.begin(); it != enemies.end(); ) {
			Enemy* enemy = *it;
			if (!enemy->isAlive()) { ++it; continue; }

			// --- A. 获取敌人判定参数 ---
			float eCx = enemy->x + enemy->width / 2.0f;
			float eCy = enemy->y + enemy->height / 2.0f;
			float enemyR = (enemy->type == eType::elf) ? 18.0f : 12.0f; // 大精灵18，小兵12
			// --- B. AABB 粗筛 (性能优化) ---
			// 如果 x 或 y 轴的投影距离超过半径之和，绝对不可能相撞
			float rSum = bulletR + enemyR;
			float dx = std::abs(bCx - eCx);
			float dy = std::abs(bCy - eCy);

			if (dx > rSum || dy > rSum) {
				++it;
				continue;
			}
			// --- C. 圆形精准判定 ---
			if (checkCircleCollide(bCx, bCy, bulletR, eCx, eCy, enemyR)) {
				enemy->hp--;
				b->alive = false;
				// 死亡逻辑
				if (enemy->hp <= 0) {
					// 生成掉落 
					EnemyManager::DropReq req;
					req.x = eCx;
					req.y = eCy;
					req.count = (enemy->type == eType::elf) ? 5 : 1;
					E.dropQueue.push_back(req);
					long long score = (enemy->type == eType::elf) ? 5000 : 1000;
					Hero::addScore(score);
					delete enemy;
					it = enemies.erase(it); 
					// 子弹已死，不需要再检测其他敌人，直接跳出内层循环，处理下一颗子弹
					goto BulletHitBreak;
				}
				// 没死，但也命中了，跳出内层循环
				goto BulletHitBreak;
			}
			++it;
		}
		continue; 
	BulletHitBreak:
		continue; 
	}

	// 敌人子弹 vs 自机 
	if (!Hero.isInvincible()) {
		float heroCx = Hero.x + Hero::getWidth() / 2;
		float heroCy = Hero.y + Hero::getHeight() / 2;
		float heroR = (float)Hero.JudgeR; 
		for (auto* b : Barr.barrList) {
			if (!b->isAlive()) continue;
			// 弹幕判定半径 
			float barrCx = b->x + Barrage::getDarkGreenWidth() / 2;
			float barrCy = b->y + Barrage::getDarkGreenHeight() / 2;
			float barrR = 10.0f; 

			if (checkCircleCollide(heroCx, heroCy, heroR, barrCx, barrCy, barrR)) {
				Hero.hit();
				b->alive = false;
				break; 
			}
		}
	}

	// 敌人 vs 自机 
	if (!Hero.isInvincible()) {
		float heroCx = Hero.x + Hero::getWidth() / 2;
		float heroCy = Hero.y + Hero::getHeight() / 2;
		float heroR = (float)Hero.JudgeR;

		for (auto* e : E.getList()) {
			if (!e->isAlive()) continue;

			float eCx = e->x + e->width / 2;
			float eCy = e->y + e->height / 2;
			float eR = (float)min(e->width, e->height) / 3; 

			if (checkCircleCollide(heroCx, heroCy, heroR, eCx, eCy, eR)) {
				Hero.hit();
				break;
			}
		}
	}
	// 符卡碰撞逻辑
	if (isSpellActive && !spellBarrages.empty()) {

		// 1. 符卡消弹：符卡弹幕 vs 敌方弹幕

		for (auto& eb : Barr.barrList) { // 敌方弹幕列表
			if (!eb->alive) continue;
			float sbx = eb->x + Barrage::getDarkGreenWidth() - Hero.x;
			float sby = eb->y + Barrage::getDarkGreenHeight() - Hero.y;
			float dist = sbx * sbx + sby * sby;

			// 简单的距离判定
			// 距离小于 20 像素就消除敌弹
			if (dist - spellRadius * spellRadius < 400) {
				eb->alive = false; // 消除敌弹
				// 符卡弹幕通常穿透，所以不消除 sb

				// 可选：生成一个消弹特效 (DropReq 里的 Score点?)
			}
		}
		

		// 2. 符卡伤害：符卡弹幕 vs 敌人

		for (auto* enemy : E.getList()) {
			if (!enemy->isAlive()) continue;
			float enx = enemy->x + enemy->width / 2 - Hero.x;
			float eny = enemy->y + enemy->height / 2 - Hero.y;
			float dist = enx * enx + eny * eny;
			if (dist - spellRadius * spellRadius < 400) {
				// 造成伤害
				enemy->hp -= 2; // 符卡伤害很高
				if (enemy->hp <= 0) enemy->alive = false;
			}
		}
		
	}
}

void Game::CastSpellCard() {
	// 防止重复释放
	if (isSpellActive) return;

	// 清理可能存在的残留
	ClearSpellBarrages();

	isSpellActive = true;
	spellRadius = 10.0f; // 初始半径为0（从自机身体里钻出来）
	spellAngle = 0.0f;  // 初始角度

	// [核心修改] 只生成 4 个弹幕
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
	spellRadius += 3.0f; // 半径扩大速度 (数值越大扩散越快)
	spellAngle += 0.1f;  // 旋转角速度

	// 获取自机中心点
	float heroCx = Hero.x + Hero::getWidth() / 2;
	float heroCy = Hero.y + Hero::getHeight() / 2;

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




	