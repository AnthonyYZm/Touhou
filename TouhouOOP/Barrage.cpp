#include "Barrage.h"
#include "Game.h"

int const Barrage::darkGreenWidth = 24;
int const Barrage::darkGreenHeight = 24;
int const Barrage::riceWidth = 9;
int const Barrage::riceHeight = 16;
int Barrage::wheelGroup = 1;
int Barrage::globalGroupID = 0;
IMAGE Barrage::imgBlue;
IMAGE Barrage::imgGreen;
IMAGE Barrage::imgRiceBlue;
IMAGE Barrage::imgRiceRed;	
IMAGE Barrage::imgBlueBig;
IMAGE Barrage::mxtsSpell;
IMAGE Barrage::imgRed;

Barrage::Barrage(float _x, float _y, BulletStyle _style) : Role(_x, _y) {
	this->style = _style;
	if (imgGreen.getwidth() == 0) loadimage(&imgGreen, L"resource/barrage/mid_bullet_darkGreen.png");
	if (imgBlue.getwidth() == 0) loadimage(&imgBlue, L"resource/barrage/blue_bullet.png");
	if (imgRiceBlue.getwidth() == 0) loadimage(&imgRiceBlue, L"resource/barrage/rice_bullet.png");
	if (imgRiceRed.getwidth() == 0) loadimage(&imgRiceRed, L"resource/barrage/rice_bullet.png");
	if (mxtsSpell.getwidth() == 0) loadimage(&mxtsSpell, L"resource/barrage/etama_big.png");
	if (imgRed.getwidth() == 0) loadimage(&imgRed, L"resource/barrage/red_bullet.png");
	t1 = 0; t2 = 0;
	vx = 0.0f; vy = 0.0f;
	speed = 4.0f;
	moveType = 0;	
	radius = 0;
	centerX = 0; centerY = 0;
	alive = false;
	currentAngle = 90.0f;
	lock = false;
	outBound = false;
	groupID = -1;
	isFriendly = false;
	omega = 0.0f; radian = 0.0f; rSpeed = 0.0f;
	this->isRotated = false;
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

BulletRect Barrage::getRect(BulletStyle s) {
	switch (s) {
	case BulletStyle::GREEN_: return { 0, 0, darkGreenWidth, darkGreenHeight };
	case BulletStyle::BLUE_: return { 0, 0, darkGreenWidth, darkGreenHeight };
	case BulletStyle::RICE_BULE: return { 100, 0, riceWidth, riceHeight };
	case BulletStyle::RICE_RED: return { 36, 0, riceWidth, riceHeight };
	case BulletStyle::RED_: return { 0, 0, darkGreenWidth, darkGreenHeight };
	case BulletStyle::BLUE_BIG: return { 0, 0, (int)(darkGreenWidth * 1.5), (int)(darkGreenWidth * 1.5) };
	}
}

int Barrage::getWidth() const { return getRect(this->style).w; }

int Barrage::getHeight() const { return getRect(this->style).h; }

void Barrage::calRotation() {
	// 只对米型弹幕旋转
	bool isRice = (style == BulletStyle::RICE_BULE || style == BulletStyle::RICE_RED);
	if (!isRice) {
		isRotated = false;
		return;
	}

	BulletRect r = getRect(this->style);
	IMAGE* imgPtr = nullptr;
	switch (style) {
	case BulletStyle::RICE_BULE: imgPtr = &imgRiceBlue; break;
	case BulletStyle::RICE_RED:  imgPtr = &imgRiceRed;  break;
	}

	if (!imgPtr || imgPtr->getwidth() == 0) return;

	// 计算旋转角度
	float angle = atan2(vy, vx);
	float theta = PI / 2.0f - angle;

	// 创建正方形画布
	int side = (std::max)(r.w, r.h);
	IMAGE squareImg(side, side);
	SetWorkingImage(&squareImg);
	// 填充透明
	DWORD* buf = GetImageBuffer(&squareImg);
	memset(buf, 0, side * side * sizeof(DWORD));
	int offsetX = (side - r.w) / 2;
	int offsetY = (side - r.h) / 2;
	SetWorkingImage(imgPtr);
	IMAGE tempImg;
	getimage(&tempImg, r.x, r.y, r.w, r.h);
	SetWorkingImage(&squareImg);
	putimage(offsetX, offsetY, &tempImg, SRCPAINT);
	SetWorkingImage(NULL);
	// 旋转
	rotateimage(&cacheImg, &squareImg, theta);
	// 修复透明度
	DWORD* pMem = GetImageBuffer(&cacheImg);
	int pixelCount = cacheImg.getwidth() * cacheImg.getheight();
	for (int i = 0; i < pixelCount; i++) {
		if ((pMem[i] & 0x00FFFFFF) != 0) {
			pMem[i] |= 0xFF000000;
		}
	}
	this->isRotated = true;
}

void Barrage::draw() {
	if (isFriendly) {
		// 大玉
		int drawW = 200;
		int drawH = 200;
		putimagePNG((int)(x - drawW / 2), (int)(y - drawH / 2), 256, 256, &mxtsSpell, 0, 0, drawW, drawH);
		return;
	}
	if (isRotated) {
		int rw = cacheImg.getwidth();
		int rh = cacheImg.getheight();
		putimagePNG((int)(x - rw / 2), (int)(y - rh / 2), rw, rh, &cacheImg, 0, 0, rw, rh);
	}
	else {
		IMAGE* imgPtr = nullptr;
		BulletRect r = getRect(this->style);
		int dstX = r.w;
		int dstY = r.h;
		switch (style) {
		case BulletStyle::GREEN_:
			imgPtr = &imgGreen;
			break;
		case BulletStyle::RICE_BULE:
			imgPtr = &imgRiceBlue;
			dstX = (int)(riceWidth * 1.2); dstY = (int)(riceHeight * 1.2);
			break;
		case BulletStyle::RICE_RED:
			imgPtr = &imgRiceRed;
			dstX = (int)(riceWidth * 1.2); dstY = (int)(riceHeight * 1.2);
			break;
		case BulletStyle::BLUE_BIG:
			imgPtr = &imgBlue;
			r.w = darkGreenWidth;
			r.h = darkGreenHeight;
			dstX = (int)(darkGreenWidth * 1.5f);
			dstY = (int)(darkGreenHeight * 1.5f);
			break;
		case BulletStyle::BLUE_:
			imgPtr = &imgBlue;
			break;
		case BulletStyle::RED_:
			imgPtr = &imgRed;
			break;
		default:
			break;
		}
		if (imgPtr && imgPtr->getwidth() > 0) {
			putimagePNG((int)(x - dstX / 2), (int)(y - dstY / 2), r.w, r.h, imgPtr, r.x, r.y, dstX, dstY);
		}
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
		if (GetTickCount() < t1) {
			return;
		}
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


void Barrage::Normal(Enemy& e, float speed, BulletStyle s) {
	if (!e.isAlive() || !e.fire) return;
	Barrage* newBarrage = new Barrage(e.x, e.y, s);
	newBarrage->vx = 0;
	newBarrage->vy = speed;
	newBarrage->alive = true;
	barrList.push_back(newBarrage);
	Game::Effects.spawn(EffectType::CREATE_BARRAGE, e.x, e.y);
}
//straight

void Barrage::straightMill(Enemy& e, float speed, int omega, int num, int x0, int y0, int dir, BulletStyle s) {
	if (e.alive && e.fire) {
		static float spin = 0;
		for (int i = 0; i < num; ++i) {
			Barrage* newBarrage = new Barrage((float)x0, (float)y0, s);
			float angle = i * (360.0f / num) + spin;
			float rad = angle * PI / 180.0f;
			if (dir == 0) rad = -rad;
			newBarrage->vx = speed * cos(rad);
			newBarrage->vy = speed * sin(rad);
			newBarrage->calRotation();
			newBarrage->alive = true;
			barrList.push_back(newBarrage);
			Game::Effects.spawn(EffectType::CREATE_BARRAGE, x0, y0);
			Game::Audio.play(L"barrage");
		}
		spin += omega;
	}
}

void Barrage::straightMill2(Enemy& e, float speed, int omega, int num, int x0, int y0, int dir, BulletStyle s) {
	if (e.alive && e.fire) {
		static float spin = 0;
		for (int i = 0; i < num; ++i) {
			Barrage* newBarrage = new Barrage((float)x0, (float)y0, s);
			float angle = i * (360.0f / num) + spin;
			float rad = angle * PI / 180.0f;
			if (dir == 0) rad = -rad;
			newBarrage->vx = speed * cos(rad);
			newBarrage->vy = speed * sin(rad);
			newBarrage->calRotation();
			newBarrage->alive = true;
			barrList.push_back(newBarrage);
			Game::Effects.spawn(EffectType::CREATE_BARRAGE, x0, y0);
			Game::Audio.play(L"barrage");
		}
		spin += omega;
	}
}

void Barrage::directionalMill(Enemy& e, float speed, float baseAngle, int num, int x0, int y0, int dir, BulletStyle s) {
	if (e.isAlive() && e.fire) {
		for (int i = 0; i < num; ++i) {
			Barrage* newBarrage = new Barrage((float)x0, (float)y0, s);
			float angle = i * (360.0f / num) + baseAngle;
			float rad = angle * PI / 180.0f;	
			newBarrage->vx = speed * cos(rad);
			newBarrage->vy = speed * sin(rad);
			newBarrage->calRotation();
			newBarrage->alive = true;
			newBarrage->moveType = 0; 
			barrList.push_back(newBarrage);
			Game::Effects.spawn(EffectType::CREATE_BARRAGE, x0, y0);
			Game::Audio.play(L"barrage");
		}
	}
}

void Barrage::fireWork(Enemy& e, float speed, int num, int x0, int y0, BulletStyle s) {
	if (e.alive && e.fire) {
		for (int i = 0; i < num; ++i) {
			Barrage* newBarrage = new Barrage((float)x0, (float)y0, s);
			float angle = i * (360.0f / num);
			float rad = angle * PI / 180.0f;
			newBarrage->vx = speed * cos(rad);
			newBarrage->vy = speed * sin(rad);
			newBarrage->calRotation();
			newBarrage->alive = true;
			barrList.push_back(newBarrage);
			Game::Effects.spawn(EffectType::CREATE_BARRAGE, x0, y0);
			Game::Audio.play(L"barrage");
		}
	}
}

void Barrage::circleMill(Enemy& e, float speed, int r, int num, int x0, int y0, BulletStyle s) {
	if (e.isAlive() && e.fire) {
		float R = (float)r;
		for (int i = 0; i < num; ++i) {
			Barrage* newBarrage = new Barrage((float)x0, (float)y0, s);
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
			Game::Effects.spawn(EffectType::CREATE_BARRAGE, x0, y0);
			Game::Audio.play(L"barrage");
		}
	}
}

void Barrage::pincerAim(Enemy& e, float targetX, float targetY, float speed, int spacing, int pairNum, int x0, int y0, BulletStyle s) {
	if (e.isAlive() && e.fire) {

		for (int i = 1; i <= pairNum; ++i) {
			// 当前的横向偏移量
			int offset = i * spacing;
			// 左侧子弹
			Barrage* leftB = new Barrage((float)(x0 - offset), (float)y0, s);
			float angleL = atan2(targetY - y0, targetX - (x0 - offset));
			leftB->vx = speed * cos(angleL);
			leftB->vy = speed * sin(angleL);
			leftB->calRotation();	
			leftB->alive = true;
			leftB->moveType = 0; 
			barrList.push_back(leftB);
			Game::Effects.spawn(EffectType::CREATE_BARRAGE, x0 - offset, y0);

			// 右侧子弹
			Barrage* rightB = new Barrage((float)(x0 + offset), (float)y0, s);
			float angleR = atan2(targetY - y0, targetX - (x0 + offset));
			rightB->vx = speed * cos(angleR);
			rightB->vy = speed * sin(angleR);
			rightB->calRotation();
			rightB->alive = true;
			rightB->moveType = 0; 
			barrList.push_back(rightB);
			Game::Effects.spawn(EffectType::CREATE_BARRAGE, x0 + offset, y0);
			Game::Audio.play(L"barrage");
		}
	}
}

void Barrage::randomRain(float speed, BulletStyle s) {
	int rndX = rand() % (int(LeftEdge), int(screenWidth + LeftEdge - darkGreenWidth));
	Barrage* newBarrage = new Barrage((float)rndX, (float)TopEdge, s);
	newBarrage->vx = 0;          
	newBarrage->vy = speed;    
	newBarrage->alive = true;
	newBarrage->moveType = 0;    
	barrList.push_back(newBarrage);
	Game::Effects.spawn(EffectType::CREATE_BARRAGE, rndX, TopEdge);
	Game::Audio.play(L"barrage");
}

void Barrage::fiveStar(Enemy& e, int currentStep, int totalSteps, float R_orbit, float r_star, float stretchSpeed,
	float normalAcc, DWORD releaseTime, int starCount, int x0, int y0, BulletStyle s) {
	if (!e.isAlive()) return;
	// 计算归一化进度 
	float normalizedProgress = (float)currentStep / (float)totalSteps * 5.0f;
	int edgeIdx = (int)normalizedProgress;
	if (edgeIdx >= 5) edgeIdx = 4;
	float t = normalizedProgress - (float)edgeIdx;

	// 确定顶点 
	int indices[] = { 4, 1, 3, 0, 2, 4 };
	POINTFLOAT pStart = getStarVertex(indices[edgeIdx], r_star);
	POINTFLOAT pEnd = getStarVertex(indices[edgeIdx + 1], r_star);

	// 计算位置
	float lx = pStart.x + (pEnd.x - pStart.x) * t;
	float ly = pStart.y + (pEnd.y - pStart.y) * t;
	// 计算参数
	float dx = pEnd.x - pStart.x;
	float dy = pEnd.y - pStart.y;
	float len = sqrt(dx * dx + dy * dy);
	float ux = (len != 0) ? dx / len : 0;
	float uy = (len != 0) ? dy / len : 0;

	// 切向速度
	float stretchFactor = -8.0f;
	float vx_tan = ux * stretchFactor * stretchSpeed;
	float vy_tan = uy * stretchFactor * stretchSpeed;
	float nx = -uy;
	float ny = ux;

	// 确保法向量向外
	float mx = (pStart.x + pEnd.x) / 2.0f;
	float my = (pStart.y + pEnd.y) / 2.0f;
	if (mx * nx + my * ny > 0) {
		nx = -nx;
		ny = -ny;
	}
	float zeroThreshold = 1.0f / 3.0f;
	float scale = 0.8f;
	float accFactor = (zeroThreshold - t) * scale;
	float currentAcc = normalAcc * accFactor;
	float ax = nx * currentAcc;
	float ay = ny * currentAcc;

	// 生成子弹 
	for (int i = 0; i < starCount; ++i) {
		float orbitAng = -PI / 2.0f + i * (2.0f * PI / (float)starCount);		
		float cx = x0 + R_orbit * cos(orbitAng);
		float cy = y0 + R_orbit * sin(orbitAng);
		Barrage* b = new Barrage((float)(cx + lx), (float)(cy + ly), s);
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
		Game::Effects.spawn(EffectType::CREATE_BARRAGE, cx + lx, cy + ly);
		Game::Audio.play(L"barrage");
	}
}

void Barrage::update() {
	// 统计还活跃的组
	std::set<int> groupsInside;
	for (auto* b : barrList) {
		b->move(); 
		b->collision();
		if (b->lock && !b->outBound) {
			groupsInside.insert(b->groupID);
		}
	}
	// 解锁已完全出界的组
	for (auto* b : barrList) {
		if (b->lock && b->groupID != -1) {
			if (groupsInside.find(b->groupID) == groupsInside.end()) {
				b->lock = false;
			}
		}
	}
	// 删除
	for (auto it = barrList.begin(); it != barrList.end(); ) {
		Barrage* b = *it;
		if (b->isAlive()) {
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
