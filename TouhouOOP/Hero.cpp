#include "Hero.h"

int const Hero::heroWidth = 32;
int const Hero::heroHeight = 45;
long long Hero::score = 0;
static const int VISUAL_W = 48;
static const int VISUAL_H = 67;

Hero::Hero(float _x, float _y) : Role(_x, _y, 1) {
	loadimage(&hero, L"resource/hero/Reimu.png");
	frame = 8;
	col = 0;
	row = 0;
	JudgeR = 6; //Judge point 
	Speed = 6.0f; 
	//Hero frame rate
	th1 = GetTickCount(); th2 = 0;
	lives = 100;      
	power = 0;
	score = 0;
	invincible = false;
	invincibleEnd = 0;
	bombCount = 5;
	maxBomb = 5;
}

void Hero::draw() {
	if (invincible) {
		if ((GetTickCount() / 50) % 2 == 0) return; 
	}

	th2 = GetTickCount();
	int sx = row * heroWidth;
	int sy = 0;
	if (GetAsyncKeyState(VK_LEFT)) { sy = 49; }
	else if (GetAsyncKeyState(VK_RIGHT)) { sy = 98; }
	int drawX = (int)(x - VISUAL_W / 2);
	int drawY = (int)(y - VISUAL_H / 2);
	putimagePNG(drawX, drawY, heroWidth, heroHeight, &hero, sx, sy, VISUAL_W, VISUAL_H);
	if (th2 - th1 > 80) {
		row = (row + 1) % frame;
	th1 = th2;
	}
}

void Hero::JudgePoint() {
	setlinecolor(RED);
	setfillcolor(WHITE);
	fillcircle((int)x, (int)y, JudgeR);
}	

// 控制自机
void Hero::control(float speed) {
	if (GetAsyncKeyState(VK_LEFT) && GetAsyncKeyState(VK_UP) || GetAsyncKeyState(VK_RIGHT) && GetAsyncKeyState(VK_UP) ||
		GetAsyncKeyState(VK_LEFT) && GetAsyncKeyState(VK_DOWN) || GetAsyncKeyState(VK_RIGHT) && GetAsyncKeyState(VK_DOWN)) {
		speed /= 1.414f;    
	}
	if (GetAsyncKeyState(VK_LSHIFT) & 0x8000) {
		speed *= 0.6f; // 按下Shift键减速
		JudgePoint();
	}
	if (GetAsyncKeyState(VK_UP) & 0x8000) {
		y -= speed;
	}
	if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
		y += speed;
	}
	if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
		x -= speed;
	}
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
		x += speed;
	}

}
// 移动，边界检测
void Hero::move() {
	if (invincible && GetTickCount() > invincibleEnd) {
		invincible = false;
	}
	control(Speed);
	float halfW = VISUAL_W / 2.0f;
	float halfH = VISUAL_H / 2.0f;
	if (x < LeftEdge + halfW) x = LeftEdge + halfW;
	if (x > (LeftEdge + WIDTH) - halfW) x = (LeftEdge + WIDTH) - halfW;
	if (y < TopEdge + halfH) y = TopEdge + halfH;
	if (y > (TopEdge + HEIGHT) - halfH) y = (TopEdge + HEIGHT) - halfH;
}

void Hero::hit() {
	if (invincible) return; 
	lives--;

	if (lives >= 0) {
		x = (LeftEdge + WIDTH) / 2.0f;
		y = TopEdge + HEIGHT - 100.0f;
		invincible = true;
		invincibleEnd = GetTickCount() + 3000;
	}
	else {
		// Game Over 
		alive = false;
	}
}

bool Hero::tryUseBomb() {
	if (bombCount > 0) {
		bombCount--;
		return true;
	}
	return false;
}

void Hero::addBomb(int b) {
	bombCount += b;
	if (bombCount > maxBomb) bombCount = maxBomb;
}

void Hero::addPower(int p) {
	power += p;
	if (power > 128) power = 128; 
}

void Hero::addScore(int s) {
	score += s;
}

