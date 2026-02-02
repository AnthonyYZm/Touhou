#include "Bullet.h"
#include "Enemy.h"

int const Bullet::bulletWidth = 12;
int const Bullet::bulletHeight = 14;
IMAGE Bullet::bullet1;

Bullet::Bullet(float _x, float _y) : Role(_x, _y) {
    if (bullet1.getwidth() == 0) {
        loadimage(&bullet1, L"resource/bullet/bullet1.png");
    }
	t1 = 0;
	t2 = 0;
	speed = 20.0f;
	alive = false;
	fire = false;	
    vx = 0;
	vy = -speed;
}

Bullet::~Bullet() {
	for (auto* b : bulletList) {
		delete b;
	}
	bulletList.clear();
}

void Bullet::draw() {
    putimagePNG((int)(x - bulletWidth * 0.75), (int)(y - bulletHeight * 0.75),
        bulletWidth, bulletHeight, &bullet1, 34, 133, (int)(bulletWidth * 1.5), (int)(bulletHeight * 1.5));
}

void Bullet::move() {
	if (alive) {
        x += vx;
        y += vy;
        if (y <= TopEdge || y >= HEIGHT || x <= LeftEdge || x >= Right) {
            alive = false;
        }
	}
}

void Bullet::createBullet(Hero* hero, int type, const std::vector<Enemy*>& enemies) {
	auto now = clock::now();
    if (fire) {
        if (now - next_fire > std::chrono::milliseconds(200)) {
            next_fire = now;
        }
        while (now >= next_fire) {
            std::chrono::duration<float> diff = now - next_fire;
            float dt = diff.count();
            float currentSpeed = 20.0f;
            float offsetDistance = (currentSpeed * 60.0f) * dt;
            float offset = 3;
            float hx = hero->x - offset;
            float hy = hero->y - 10.0f - offsetDistance - offset;
            // 获取当前火力
            int p = hero->getPower();

            // --- 模式 1: 初始状态 (1发) ---
            if (p < 30) {
                Bullet* b = new Bullet(hx, hy);
                b->setVelocity(0, -b->speed);
                b->alive = true;
                bulletList.push_back(b);
            }
            // --- 模式 2: 双发 (火力 >= 10) ---
            else if (p >= 30 && p < 50) {
                Bullet* b1 = new Bullet(hx - 8, hy);
                Bullet* b2 = new Bullet(hx + 8, hy);
                b1->setVelocity(0, -b1->speed);
                b2->setVelocity(0, -b2->speed);
                b1->alive = true; b2->alive = true;
                bulletList.push_back(b1);
                bulletList.push_back(b2);
            }
            // --- 模式 3: 三发散射 (火力 >= 30) ---
            else {
                Bullet* center1 = new Bullet(hx - 8, hy);
                Bullet* center2 = new Bullet(hx + 8, hy);
                center1->setVelocity(0, -center1->speed);
                center2->setVelocity(0, -center2->speed);
                center1->alive = true; center2->alive = true;
                bulletList.push_back(center1);
                bulletList.push_back(center2);

                // 2. 计算诱导子弹的目标
                Enemy* target = nullptr;
                float minDistSq = 100000000.0f; // 很大的初始值

                // 寻找最近的存活敌人
                for (auto* e : enemies) {
                    if (!e->isAlive()) continue;
                    float dx = e->x - hx;
                    float dy = e->y - hy;
                    float distSq = dx * dx + dy * dy;
                    if (dy < 0 && distSq < minDistSq) {
                        minDistSq = distSq;
                        target = e;
                    }
                }

                Bullet* leftB = new Bullet(hx - 24, hy + 5); // 左翼发射点
                Bullet* rightB = new Bullet(hx + 24, hy + 5); // 右翼发射点

                float speed = leftB->speed; // 基础速度

                if (target) {
                    // --- 锁定目标模式 ---
                    // 计算指向敌人的角度
                    // atan2(y, x) 返回弧度
                    float angleL = atan2(target->y - leftB->y, target->x - leftB->x);
                    float angleR = atan2(target->y - rightB->y, target->x - rightB->x);

                    leftB->setVelocity(speed * cos(angleL), speed * sin(angleL));
                    rightB->setVelocity(speed * cos(angleR), speed * sin(angleR));
                }
                else {
                    // --- 无目标模式 (斜向发射) ---
                    // 左边向左上 (-90 - 20度)，右边向右上 (-90 + 20度)
                    // -90度 = -PI/2
                    float offsetAngle = 20.0f * (PI / 180.0f);
                    float baseAngle = -PI / 2.0f;

                    float angleL = baseAngle - offsetAngle;
                    float angleR = baseAngle + offsetAngle;

                    leftB->setVelocity(speed * cos(angleL), speed * sin(angleL));
                    rightB->setVelocity(speed * cos(angleR), speed * sin(angleR));
                }

                leftB->alive = true;
                rightB->alive = true;
                bulletList.push_back(leftB);
                bulletList.push_back(rightB);
            }

            next_fire = now + fire_cd;
        }
        fire = false;
    }
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

