#include "Hero.h"

Hero::Hero(float _x, float _y) : Role(_x, _y, 1) {
	loadimage(&hero, L"resource/hero/Reimu.png");
	heroWidth = 32;
	heroHeight = 45;
	frame = 8;
	col = 0;
	row = 0;
	JudgeR = 6; //Judge point 判定点
	Speed = 4.5f; 
	//Hero frame rate
	th1 = GetTickCount();
	th2 = 0;
}

/*Draw hero
	Use sprite sheet for animation	 精灵表
*/
void Hero::draw() {
	th2 = GetTickCount();
	int sx = row * heroWidth;
	int sy = 0;
	if (GetAsyncKeyState(VK_LEFT)) { sy = 49; }
	else if (GetAsyncKeyState(VK_RIGHT)) { sy = 98; }
	putimagePNG((int)x, (int)y, heroWidth, heroHeight, &hero, sx, sy);
	if (th2 - th1 > 80) {
		row = (row + 1) % frame;
	th1 = th2;
	}
}

void Hero::JudgePoint() {
	setlinecolor(RED);
	setfillcolor(WHITE);
	fillcircle((int)(x + heroWidth / 2), (int)(y + heroHeight / 2), JudgeR);
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
	if (x >= LeftEdge && x <= WIDTH - heroWidth && y >= TopEdge && y <= HEIGHT - heroHeight) {
		control(Speed); //Set hero's speed here
	}

	if (x <= LeftEdge) x = LeftEdge;
	if (x >= WIDTH - heroWidth) x = (float)(WIDTH - heroWidth);
	if (y <= TopEdge) y = TopEdge;
	if (y >= HEIGHT - heroHeight) y = (float)(HEIGHT - heroHeight);
}

