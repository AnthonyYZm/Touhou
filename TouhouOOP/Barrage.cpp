#include "Barrage.h"

int const Barrage::darkGreenWidth = 24;
int const Barrage::darkGreenHeight = 24;
int Barrage::wheelGroup = 1;
int Barrage::globalGroupID = 0;

Barrage::Barrage(float _x, float _y) : Role(_x, _y) {
	loadimage(&barr1, L"resource/barrage/mid_bullet_darkGreen.png");
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
}

Barrage::~Barrage() {
	for (auto* b : barrList) {
		delete b;
	}
	barrList.clear();
}

void Barrage::reset() {
	t1 = GetTickCount();
	currentAngle = 90.0f;
}

void Barrage::draw() {
	putimagePNG((int)x, (int)y, darkGreenWidth, darkGreenHeight, &barr1, 0, 0);
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
}

void Barrage::collision() {
	if (x < 0 || x > WIDTH || y < 0 || y > HEIGHT) {
		outBound = true;	
		if (!lock) alive = false;
	}
}


void Barrage::Normal(Enemy& e, float speed) {
	if (!e.isAlive() || !e.fire) return;
	Barrage* newBarrage = new Barrage(e.x + Enemy::getNormalWidth() / 2 - darkGreenWidth / 2, e.y + Enemy::getNormalHeight() / 2 - darkGreenHeight / 2);
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
			newBarrage->radius = 0; // 初始半径
			newBarrage->rSpeed = speed;

			// 修正：如果 vl 是角速度直接赋值，如果是线速度则要注意除零
			// 这里假设传入的 vl 是想要的角速度
			newBarrage->omega = vl;

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

		float aimX = targetX + Hero::getWidth() / 2;
		float aimY = targetY + Hero::getHeight() / 2;

		for (int i = 1; i <= pairNum; ++i) {
			// 当前的横向偏移量
			int offset = i * spacing;
			// 左侧子弹
			Barrage* leftB = new Barrage(x0 - offset, y0); 
			float angleL = atan2(aimY - y0, aimX - (x0 - offset)); 
			leftB->vx = speed * cos(angleL);
			leftB->vy = speed * sin(angleL);
			leftB->alive = true;
			leftB->moveType = 0; 
			barrList.push_back(leftB);

			// 右侧子弹
			Barrage* rightB = new Barrage(x0 + offset, y0);
			float angleR = atan2(aimY - y0, aimX - (x0 + offset)); 
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
