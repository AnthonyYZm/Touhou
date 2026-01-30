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
}

int getEnemyHeight(Enemy& e) {
    if (e.type == eType::normal) return Enemy::normalHeight;
    else if (e.type == eType::elf) return Enemy::elfHeight;
    else if (e.type == eType::boss) return Enemy::bossHeight;
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
	hp = 1; 
	col = 0; frame = 0; row = 0;
	enemyX = 0; enemyY = 0;
    te1 = 0, te2 = 0;	
	fire = true;
	type = eType::normal;
	birthTime = GetTickCount();
	mover = nullptr;	
	actionState = 0;
}

void Enemy::EnemyX() { //set round's enemyX

}

void Enemy::draw() {
    te2 = GetTickCount();

    // [修正1] 定义一个指针用于切换图片，默认指向普通敌人图片
    IMAGE* currentImg = &img;

    // 默认的剪裁宽高 (Source Size)
    int clipW = width;
    int clipH = height;

    // --- 1. 根据类型选择贴图和参数 ---
    if (type == eType::boss) {
        // [修正2] 如果是 Boss，必须切换图片源！
        currentImg = &sanae;

        // Boss 原始素材的单帧宽高 (根据你的描述 256x256, 4列3行)
        // 256/4 = 64, 256/3 ≈ 85
        width = 55;
        height = 76;

        // Boss 逻辑：根据 actionState 决定行 (row)
        // row 0: 静止, row 1: 进场, row 2: 攻击
        int currentRow = 0;
        if (actionState == 1) currentRow = 1; // 进场/移动
        else if (actionState == 2) currentRow = 2; // 攻击

        // 计算 sx (列动画)
        // 注意：最后一行(攻击)只有3帧，其他行4帧
        int maxFrames = (currentRow == 2) ? 3 : 4;
        clipW = 64;
        if (currentRow == 0) {
            clipH = 60;
            sx = (row % maxFrames) * clipW;
            sy = 0;
        }
        else if (currentRow == 1) {
			clipH = 54;
			sx = (row % maxFrames) * clipW;
            sy = 67;
        }
        else if (currentRow == 2) {
			clipH = 76;
			sx = (row % maxFrames) * clipW;
            sy = 130;
        }
    }
    // 普通敌人/精灵的逻辑 (保持原样，使用 &img)
    else {
        // 你的代码中 normalWidth/elfWidth 是静态常量，这里不需要重新赋值
        // sx, sy 已经在 EnemyManager::drawAll 中计算好了，这里直接用
        clipW = width;
        clipH = height;
    }

    // --- 2. 绘制 ---
    // [修正3] 
    // 参数3,4 (width, height): 剪裁大小 (Source Size) -> 对应你的 clipW, clipH
    // 参数5 (currentImg): 图片源 -> [关键] 动态切换了 &sanae 或 &img
    // 参数8,9 (width*1.5): 实际显示大小 (Destination Size) -> 放大 1.5 倍

    putimagePNG((int)x, (int)y, clipW, clipH, currentImg, sx, sy, (int)(clipW * 1.5), (int)(clipH * 1.5));

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






