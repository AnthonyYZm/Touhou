#include "Barrage.h"

int const Barrage::darkGreenWidth = 24;
int const Barrage::darkGreenHeight = 24;
int Barrage::wheelGroup = 1;
int Barrage::globalGroupID = 0;
IMAGE Barrage::barr1;
IMAGE Barrage::mxtsSpell;

Barrage::Barrage(float _x, float _y) : Role(_x, _y) {
	if (barr1.getwidth() == 0) {
		loadimage(&barr1, L"resource/barrage/mid_bullet_darkGreen.png");
	}
	if (mxtsSpell.getwidth() == 0) {
		loadimage(&mxtsSpell, L"resource/barrage/etama_big.png");
	}
	t1 = 0;
	t2 = 0;
	vx = 0.0f;
	vy = 0.0f;
	speed = 4.0f;
	moveType = 0;	
	radius = 0;
	centerX = 0;
	centerY = 0;
	alive = false;
	currentAngle = 90.0f;
	lock = false;
	outBound = false;
	groupID = -1;
	isFriendly = false;
}

Barrage::~Barrage() {
	for (auto* b : barrList) {
		delete b;
	}
	barrList.clear();
}

// 辅助函数：计算五角星顶点坐标
POINTFLOAT getStarVertex(int index, float r) {
	float angle = -PI / 2.0f + index * (2.0f * PI / 5.0f);
	return { r * cos(angle), r * sin(angle) };
}

void Barrage::reset() {
	t1 = GetTickCount();
	currentAngle = 90.0f;
}

void Barrage::draw() {
	if (isFriendly) {
		// 大玉 (256x256)，显示大小 200x200
		int drawW = 200;
		int drawH = 200;
		putimagePNG((int)(x - drawW / 2), (int)(y - drawH / 2), 256, 256, &mxtsSpell, 0, 0, drawW, drawH);
	}
	else {
		// 普通中弹 (24x24)
		int halfW = darkGreenWidth / 2;
		int halfH = darkGreenHeight / 2;
		putimagePNG((int)(x - halfW), (int)(y - halfH), darkGreenWidth, darkGreenHeight, &barr1, 0, 0, darkGreenWidth, darkGreenHeight);
	}
}

void Barrage::move() {
	if (!alive) return;
	if (moveType == 0) {
		x += vx;
		y += vy; 
	}
	else if (moveType == 1) {
		// Circle
		currentAngle += omega;
		x = centerX + radius * cos(currentAngle);
		y = centerY + radius * sin(currentAngle);
	}
	else if (moveType == 2) {
		currentAngle += omega;
		radius += rSpeed;
		x = centerX + radius * cos(currentAngle);
		y = centerY + radius * sin(currentAngle);
	}
	else if (moveType == 3) {
		// 阶段1: 悬停等待 
		if (GetTickCount() < t1) {
			return;
		}
		// 阶段2: 物理释放
		vx += centerX;
		vy += centerY;
		x += vx;
		y += vy;
	}
}

void Barrage::collision() {
	if (x < 0 || x > WIDTH || y < 0 || y > HEIGHT) {
		outBound = true;	
		if (!lock) alive = false;
	}
}


void Barrage::Normal(Enemy& e, float speed) {
	if (!e.isAlive() || !e.fire) return;
	Barrage* newBarrage = new Barrage(e.x, e.y);
	newBarrage->vx = 0;
	newBarrage->vy = speed;
	newBarrage->alive = true;
	barrList.push_back(newBarrage);
}
//straight

void Barrage::straightMill(Enemy& e, float speed, int omega, int num, int x0, int y0, int dir) {
	if (e.alive && e.fire) {
		static float spin = 0;
		for (int i = 0; i < num; ++i) {
			Barrage* newBarrage = new Barrage(x0, y0);
			float angle = i * (360.0f / num) + spin;
			float rad = angle * PI / 180.0f;
			if (dir == 0) rad = -rad;
			newBarrage->vx = speed * cos(rad);
			newBarrage->vy = speed * sin(rad);
			newBarrage->alive = true;
			barrList.push_back(newBarrage);
		}
		spin += omega;
	}
}

void Barrage::directionalMill(Enemy& e, float speed, float baseAngle, int num, int x0, int y0) {
	if (e.isAlive() && e.fire) {
		for (int i = 0; i < num; ++i) {
			Barrage* newBarrage = new Barrage(x0, y0);

			// 关键：角度完全由传入的 baseAngle 决定
			float angle = i * (360.0f / num) + baseAngle;
			float rad = angle * PI / 180.0f;

			newBarrage->vx = speed * cos(rad);
			newBarrage->vy = speed * sin(rad);
			newBarrage->alive = true;
			newBarrage->moveType = 0; // 直线运动
			barrList.push_back(newBarrage);
		}
	}
}

void Barrage::fireWork(Enemy& e, float speed, int num, int x0, int y0) {
	if (e.alive && e.fire) {
		for (int i = 0; i < num; ++i) {
			Barrage* newBarrage = new Barrage(x0, y0);
			float angle = i * (360.0f / num);
			float rad = angle * PI / 180.0f;
			newBarrage->vx = speed * cos(rad);
			newBarrage->vy = speed * sin(rad);
			newBarrage->alive = true;
			barrList.push_back(newBarrage);
		}
	}
}

void Barrage::circleMill(Enemy& e, float speed, int r, int num, int x0, int y0) {
	if (e.isAlive() && e.fire) {
		float R = (float)r;
		for (int i = 0; i < num; ++i) {
			Barrage* newBarrage = new Barrage(x0, y0);
			float DAangle = (float)i * (2 * PI / num);
			newBarrage->moveType = 1;
			newBarrage->lock = true;
			newBarrage->centerX = x0 + R * cos(DAangle);
			newBarrage->centerY = y0 + R * sin(DAangle);
			newBarrage->radius = R;
			newBarrage->currentAngle = atan2(y0 - newBarrage->centerY, x0 - newBarrage->centerX);
			newBarrage->omega = speed / R;
			newBarrage->alive = true;
			barrList.push_back(newBarrage);
		}
	}
}

void Barrage::wheel(Enemy& e, float speed, float vl, int num, int x0, int y0) {
	if (e.isAlive() && e.fire) {
		float angleStep = 2 * PI / num;
		int batchID = ++globalGroupID;

		for (int i = 0; i < num; ++i) {
			Barrage* newBarrage = new Barrage(x0, y0);
			newBarrage->moveType = 2;
			newBarrage->centerX = (float)x0;
			newBarrage->centerY = (float)y0;
			newBarrage->radius = 10; // 初始半径
			newBarrage->rSpeed = speed;
			newBarrage->omega = vl / radius;

			newBarrage->currentAngle = i * angleStep;
			newBarrage->lock = true;
			newBarrage->groupID = batchID;

			// 计算第一帧
			newBarrage->x = newBarrage->centerX + newBarrage->radius * cos(newBarrage->currentAngle);
			newBarrage->y = newBarrage->centerY + newBarrage->radius * sin(newBarrage->currentAngle);
			newBarrage->alive = true;
			barrList.push_back(newBarrage);
		}
	}
}

void Barrage::pincerAim(Enemy& e, float targetX, float targetY, float speed, int spacing, int pairNum, int x0, int y0) {
	if (e.isAlive() && e.fire) {

		for (int i = 1; i <= pairNum; ++i) {
			// 当前的横向偏移量
			int offset = i * spacing;
			// 左侧子弹
			Barrage* leftB = new Barrage(x0 - offset, y0);
			float angleL = atan2(targetY - y0, targetX - (x0 - offset));
			leftB->vx = speed * cos(angleL);
			leftB->vy = speed * sin(angleL);
			leftB->alive = true;
			leftB->moveType = 0; 
			barrList.push_back(leftB);

			// 右侧子弹
			Barrage* rightB = new Barrage(x0 + offset, y0);
			float angleR = atan2(targetY - y0, targetX - (x0 + offset));
			rightB->vx = speed * cos(angleR);
			rightB->vy = speed * sin(angleR);
			rightB->alive = true;
			rightB->moveType = 0; 
			barrList.push_back(rightB);
		}
	}
}

void Barrage::randomRain(float speed) {
	int rndX = rand() % (int(LeftEdge), int(screenWidth + LeftEdge - darkGreenWidth));
	Barrage* newBarrage = new Barrage((float)rndX, TopEdge); // y 固定为 0
	newBarrage->vx = 0;          
	newBarrage->vy = speed;    
	newBarrage->alive = true;
	newBarrage->moveType = 0;    
	barrList.push_back(newBarrage);
}

void Barrage::fiveStar(Enemy& e, int currentStep, int totalSteps, float R_orbit, float r_star, float stretchSpeed, float normalAcc, DWORD releaseTime, int starCount, int x0, int y0) {
	if (!e.isAlive()) return;

	// 1. 计算归一化进度 (仍然是 * 5.0f，因为我们在画五角星)
	float normalizedProgress = (float)currentStep / (float)totalSteps * 5.0f;
	int edgeIdx = (int)normalizedProgress;
	if (edgeIdx >= 5) edgeIdx = 4;
	float t = normalizedProgress - (float)edgeIdx;

	// 2. 确定顶点 (不变)
	int indices[] = { 4, 1, 3, 0, 2, 4 };
	POINTFLOAT pStart = getStarVertex(indices[edgeIdx], r_star);
	POINTFLOAT pEnd = getStarVertex(indices[edgeIdx + 1], r_star);

	// 计算位置 (不变)
	float lx = pStart.x + (pEnd.x - pStart.x) * t;
	float ly = pStart.y + (pEnd.y - pStart.y) * t;

	// 3. 计算物理参数 (不变)
	float dx = pEnd.x - pStart.x;
	float dy = pEnd.y - pStart.y;
	float len = sqrt(dx * dx + dy * dy);
	float ux = (len != 0) ? dx / len : 0;
	float uy = (len != 0) ? dy / len : 0;

	// 切向速度
	float stretchFactor = (t - 0.5f) * 2.0f;
	float vx_tan = ux * stretchFactor * stretchSpeed;
	float vy_tan = uy * stretchFactor * stretchSpeed;

	float nx = -uy;
	float ny = ux;

	// 确保法向量向外
	float mx = (pStart.x + pEnd.x) / 2.0f;
	float my = (pStart.y + pEnd.y) / 2.0f;
	if (mx * nx + my * ny < 0) {
		nx = -nx;
		ny = -ny;
	}
	float accFactor = 1.0f - (t * 0.5f);
	float currentAcc = normalAcc * accFactor;
	float ax = nx * currentAcc;
	float ay = ny * currentAcc;

	// 4. 生成子弹 [核心修改]
	// 使用传入的 starCount 来决定生成多少个星星
	for (int i = 0; i < starCount; ++i) {
		// 角度间隔根据 starCount 自动计算
		float orbitAng = -PI / 2.0f + i * (2.0f * PI / (float)starCount);

		float cx = x0 + R_orbit * cos(orbitAng);
		float cy = y0 + R_orbit * sin(orbitAng);

		Barrage* b = new Barrage(cx + lx, cy + ly);

		b->moveType = 3;
		b->vx = vx_tan;
		b->vy = vy_tan;
		b->centerX = ax;
		b->centerY = ay;
		b->t1 = releaseTime;

		b->alive = true;
		b->lock = true;
		b->groupID = globalGroupID;

		barrList.push_back(b);
	}
}
void Barrage::update() {
	// 1. 统计还活跃的组
	std::set<int> groupsInside;
	for (auto* b : barrList) {
		b->move(); // 移动并检测 outBound
		b->collision();
		if (b->lock && !b->outBound) {
			groupsInside.insert(b->groupID);
		}
	}

	// 2. 解锁已完全出界的组
	for (auto* b : barrList) {
		if (b->lock && b->groupID != -1) {
			if (groupsInside.find(b->groupID) == groupsInside.end()) {
				b->lock = false;
			}
		}
	}

	// 3. 物理删除
	for (auto it = barrList.begin(); it != barrList.end(); ) {
		Barrage* b = *it;
		if (b->isAlive()) {
			// 只绘制在屏幕内的
			if (!b->outBound) b->draw();
			++it;
		}
		else {
			delete b;
			it = barrList.erase(it);
		}
	}
}

void Barrage::clearBarrage() {
	for (auto* b : barrList) {
		delete b;
	}
	barrList.clear();
}
