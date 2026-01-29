#include "Hero.h"

int const Hero::heroWidth = 32;
int const Hero::heroHeight = 45;
long long Hero::score = 0;

Hero::Hero(float _x, float _y) : Role(_x, _y, 1) {
	loadimage(&hero, L"resource/hero/Reimu.png");
	frame = 8;
	col = 0;
	row = 0;
	JudgeR = 6; //Judge point 判定点
	Speed = 4.5f; 
	//Hero frame rate
	th1 = GetTickCount(); th2 = 0;
	lives = 3;      // 初始 3 条命
	power = 0;
	score = 0;
	invincible = false;
	invincibleEnd = 0;
	bombCount = 100;
	maxBomb = 100;
}

/*Draw hero
	Use sprite sheet for animation	 精灵表
*/
void Hero::draw() {
	// 处理无敌闪烁效果
	if (invincible) {
		if ((GetTickCount() / 50) % 2 == 0) return; // 每50ms闪烁一次，不绘制
	}

	th2 = GetTickCount();
	int sx = row * heroWidth;
	int sy = 0;
	if (GetAsyncKeyState(VK_LEFT)) { sy = 49; }
	else if (GetAsyncKeyState(VK_RIGHT)) { sy = 98; }
	putimagePNG((int)x, (int)y, heroWidth, heroHeight, &hero, sx, sy, 48, 67.5);
	if (th2 - th1 > 80) {
		row = (row + 1) % frame;
	th1 = th2;
	}
}

void Hero::JudgePoint() {
	setlinecolor(RED);
	setfillcolor(WHITE);
	fillcircle((int)(x + 48 / 2), (int)(y + 67.5 / 2), JudgeR);
	// For collision detection, use the point (x + heroWidth/2, y + heroHeight - JudgeR)
	// This point is located at the bottom center of the hero sprite
}	

/*Hero control by arrow key*/
void Hero::control(float speed) {
	if (GetAsyncKeyState(VK_LEFT) && GetAsyncKeyState(VK_UP) || GetAsyncKeyState(VK_RIGHT) && GetAsyncKeyState(VK_UP) ||
		GetAsyncKeyState(VK_LEFT) && GetAsyncKeyState(VK_DOWN) || GetAsyncKeyState(VK_RIGHT) && GetAsyncKeyState(VK_DOWN)) {
		speed /= 1.414f;    // Adjust speed for diagonal movement	斜向运动时速度/√2
	}
	if (GetAsyncKeyState(VK_LSHIFT) & 0x8000) {
		speed *= 0.6f; // Slow down when holding Shift key	按住Shift键减速
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

/*Hero move and set boundary*/
void Hero::move() {

	if (invincible && GetTickCount() > invincibleEnd) {
		invincible = false;
	}

	if (x >= LeftEdge && x <= WIDTH - heroWidth && y >= TopEdge && y <= HEIGHT - heroHeight) {
		control(Speed); //Set hero's speed here
	}

	if (x <= LeftEdge) x = LeftEdge;
	if (x >= WIDTH - heroWidth) x = (float)(WIDTH - heroWidth);
	if (y <= TopEdge) y = TopEdge;
	if (y >= HEIGHT - heroHeight) y = (float)(HEIGHT - heroHeight);
}

void Hero::hit() {
	if (invincible) return; 
	lives--;
	// 播放被弹音效 (TODO)

	if (lives >= 0) {
		// 重置位置
		x = Right / 2 - heroWidth / 2;
		y = HEIGHT / 4 * 3 + TopEdge;

		// 给予 3 秒无敌时间
		invincible = true;
		invincibleEnd = GetTickCount() + 3000;
	}
	else {
		// Game Over 逻辑将在 Game 类中处理
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
	if (power > 128) power = 128; // 设定上限
}

void Hero::addScore(int s) {
	score += s;
}

