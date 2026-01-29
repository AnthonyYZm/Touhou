#include "Bullet.h"

int const Bullet::bulletWidth = 12;
int const Bullet::bulletHeight = 14;

Bullet::Bullet(float _x, float _y) : Role(_x, _y) {
	loadimage(&bullet1, L"resource/bullet/bullet1.png");
	t1 = 0;
	t2 = 0;
	speed = 6.5f;
	alive = false;
	fire = false;	
}

Bullet::~Bullet() {
	for (auto* b : bulletList) {
		delete b;
	}
	bulletList.clear();
}

void Bullet::draw() {
	putimagePNG((int)x, (int)y, bulletWidth, bulletHeight, &bullet1, 34, 133, bulletWidth * 1.5, bulletHeight * 1.5);
}

void Bullet::move() {
	if (alive) {
		y -= speed;
		if (y <= TopEdge || x <= LeftEdge || x >= Right) alive = false;
	}
}

void Bullet::createBullet(Hero* hero, int type) {
	auto now = clock::now();

    // 获取当前火力
    int p = hero->getPower();

    if (now >= next_fire && fire) {
        float hx = hero->x + Hero::getWidth() / 2 - Bullet::getBulletWidth() / 2;
        float hy = hero->y;

        // --- 模式 1: 初始状态 (1发) ---
        if (p < 10) {
            Bullet* b = new Bullet(hx, hy);
            b->alive = true;
            bulletList.push_back(b);
        }
        // --- 模式 2: 双发 (火力 >= 10) ---
        else if (p >= 10 && p < 30) {
            Bullet* b1 = new Bullet(hx - 8, hy);
            Bullet* b2 = new Bullet(hx + 8, hy);
            b1->alive = true; b2->alive = true;
            bulletList.push_back(b1);
            bulletList.push_back(b2);
        }
        // --- 模式 3: 三发散射 (火力 >= 30) ---
        else {
            // 中间
            Bullet* b1 = new Bullet(hx, hy - 5);
            b1->alive = true; bulletList.push_back(b1);
            // 左侧 
            Bullet* b2 = new Bullet(hx - 15, hy);
            b2->alive = true; bulletList.push_back(b2);
            // 右侧
            Bullet* b3 = new Bullet(hx + 15, hy);
            b3->alive = true; bulletList.push_back(b3);
        }
        next_fire = now + fire_cd;
    }
    fire = false;

	for (auto it = bulletList.begin(); it != bulletList.end(); ) {
		Bullet* b = *it;
		if (b->isAlive()) {
			b->move();
			b->draw();
		}
		if (!b->isAlive()) {
			delete b;
			it = bulletList.erase(it);
		}
		else {
			++it;
		}
	}
	//printf("Bullet List Size: %d\n", bulletList.size());
}

