#pragma once
#include "Role.h"
#include "Hero.h"

/// <summary>
/// @brief Bullet class
/// @details Inherited from Role class, represents bullets fired by the hero
/// </summary>

class Enemy;

class Bullet : public Role {

	static const int bulletWidth;
	static const int bulletHeight;
	float speed;
	float vx, vy;
	DWORD t1, t2;
	static IMAGE bullet1;
	using clock = std::chrono::steady_clock;
	clock::time_point next_fire = clock::now();    
	std::chrono::milliseconds fire_cd{ 100 };

public:
	std::vector<Bullet*> bulletList;

	Bullet(float _x = 0, float _y = 0);
	~Bullet();

	void draw() override;
	void move() override;
	void createBullet(Hero* hero, int type, const std::vector<Enemy*>& enemies);
	bool isAlive() const { return alive; }
	void setFire(bool f) { fire = f; }
	void setVelocity(float _vx, float _vy) { vx = _vx; vy = _vy; }
	static int getBulletWidth() { return bulletWidth; }
	static int getBulletHeight() { return bulletHeight; }
};


