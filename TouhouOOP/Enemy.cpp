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
    IMAGE* currentImg = &img; // 默认

    // 1. 确定图源和剪裁坐标 (sx, sy)
    if (type == eType::boss) {
        currentImg = &sanae;
		texWidth = bossWidth;
		texHeight = bossHeight;
        // 计算行 (Row)
        int currentRow = 0;
        if (actionState == 1) currentRow = 1;
        else if (actionState == 2) currentRow = 2;

        // 计算列 (Col)
        int maxFrames = (currentRow == 2) ? 3 : 4;
        sx = (row % maxFrames) * texWidth; // 使用 texWidth 切图
        sy = currentRow * texHeight;       // 使用 texHeight 切图
    }
    else {
        // Normal/Elf 的 sx/sy 逻辑保持不变 (通常在 Manager 里算好了)
        // ...
    }

    int drawX = (int)(x - width / 2.0f);
    int drawY = (int)(y - height / 2.0f);

    putimagePNG(
        drawX,      // 屏幕 X (左上角)
        drawY,      // 屏幕 Y (左上角)
        texWidth,   // 图片剪裁宽 (原始素材大小)
        texHeight,   // 图片剪裁高
        currentImg, // 图片源
        sx, sy, 
        width,
        height// 图片剪裁起始
    );
    // 动画帧更新
    if (te2 - te1 > 80) {
        int maxF = (frame == 0) ? 4 : frame; // 防止除零保护
        row = (row + 1) % maxF;
        te1 = te2;
    }
}

void Enemy::move() {
	if (!alive) return;

	// 计算存活时间
	int t = GetTickCount() - birthTime;

	// 如果配置了策略，执行策略
	if (mover) {
		mover(this, t);
	}
	else {
		// [原有逻辑兜底] 
		y += 2.0f; 
	}
}





