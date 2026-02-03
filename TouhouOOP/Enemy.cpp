#include "Enemy.h"

int const Enemy::normalWidth = 32;
int const Enemy::normalHeight = 27;
int const Enemy::elfWidth = 64;
int const Enemy::elfHeight = 50;
int const Enemy::bossWidth = 64;
int const Enemy::bossHeight = 60;
IMAGE Enemy::img;
bool Enemy::resLoaded = false;
IMAGE Enemy::sanae;

int getEnemyWidth(Enemy& e) {
    if (e.type == eType::normal) return Enemy::normalWidth;
    else if (e.type == eType::elf) return Enemy::elfWidth;
	else if (e.type == eType::boss) return Enemy::bossWidth;
	else return 0;  
}

int getEnemyHeight(Enemy& e) {
    if (e.type == eType::normal) return Enemy::normalHeight;
    else if (e.type == eType::elf) return Enemy::elfHeight;
    else if (e.type == eType::boss) return Enemy::bossHeight;
	else return 0;
}

std::map<eType, float> Enemy::speedMap = {
	{eType::normal, 4.5f},
	{eType::elf, 4.5f}
};

Enemy::Enemy(float _x, float _y, int _hp) : Role(_x, _y, _hp) {
	if (!resLoaded) {
		loadimage(&img, L"resource/enemy/enemy.png");
		loadimage(&sanae, L"resource/other/stg5enm.png");
		resLoaded = true;
	}
    hp = (_hp > 0) ? _hp : 1;
	col = 0; frame = 0; row = 0;
	enemyX = 0; enemyY = 0;
    te1 = 0, te2 = 0;
	fire = true;
	type = eType::normal;
    texWidth = 48;
    texHeight = 27;
    width = texWidth;
    height = texHeight;
	birthTime = GetTickCount();
	mover = nullptr;	
	actionState = 0;
}

void Enemy::EnemyX() { //set round's enemyX

}

void Enemy::draw() {
    te2 = GetTickCount();
    IMAGE* currentImg = &img; 

    // 确定图源和剪裁坐标 (sx, sy)
    if (type == eType::boss) {
        currentImg = &sanae;
		texWidth = bossWidth;
		texHeight = bossHeight;
        // Row
        int currentRow = 0;
        if (actionState == 1) currentRow = 1;
        else if (actionState == 2) currentRow = 2;
        // Col
        int maxFrames = (currentRow == 2) ? 3 : 4;
        sx = (row % maxFrames) * texWidth; 
        sy = currentRow * texHeight;       
    }
    else {
    }
    int drawX = (int)(x - width / 2.0f);
    int drawY = (int)(y - height / 2.0f);
    putimagePNG(drawX, drawY, texWidth, texHeight, currentImg, sx, sy, width, height);

    if (te2 - te1 > 80) {
        int maxF = (frame == 0) ? 4 : frame; 
        row = (row + 1) % maxF;
        te1 = te2;
    }
}

void Enemy::move() {
	if (!alive) return;
	int t = GetTickCount() - birthTime;
	if (mover) {
		mover(this, t);
	}
	else {
		y += 2.0f; 
	}
}





