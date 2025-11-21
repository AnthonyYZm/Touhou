#pragma once
#include "Role.h"
#include "Hero.h"

class Bullet : public Role {
	float bulletX;
	float bulletY;
	int bulletWidth;
	int bulletHeight;
	float speed;
	DWORD t1, t2;
	IMAGE bullet1;
	using clock = std::chrono::steady_clock;
	clock::time_point next_fire = clock::now();    
	std::chrono::milliseconds fire_cd{ 100 };

public:

	std::vector<Bullet> bulletList;
	Bullet(float _x = 0, float _y = 0);
	void draw() override;
	void move();
	void createBullet(Hero* hero, int type);
	float getBulletX() { return bulletX; }
	float getBulletY() { return bulletY; }
	bool isAlive() const { return alive; }
	void setFire(bool f) { fire = f; }
};


